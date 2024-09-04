/**
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @reference https://github.com/microsoft/APSI/sender/sender_db.cpp
 * @license MIT license
 * @change optimize the implementation of some functions & replace the instance of oprf
 *
 * @file SenderDB.h
 * @author: shawnhe
 * @date 2022-11-7
 *
 */

// APSI
#include <apsi/psi_params.h>
#include <apsi/util/db_encoding.h>
#include <apsi/util/label_encryptor.h>
#include <apsi/util/utils.h>

// Kuku
#include "kuku/locfunc.h"

// SEAL
#include "seal/util/common.h"
#include "seal/util/streambuf.h"

#include <tbb/parallel_for.h>
#include <numeric>
#include <utility>

#include "SenderDB.h"
#include "ppc-psi/src/labeled-psi/Common.h"

#include "../protocol/Protocol.h"

using namespace ppc;
using namespace ppc::crypto;
using namespace ppc::psi;

/**
Creates and returns the vector of hash functions similarly to how Kuku 2.x sets
them internally.
*/
inline std::vector<kuku::LocFunc> hashFunctions(const apsi::PSIParams& params)
{
    std::vector<kuku::LocFunc> result;
    for (uint32_t i = 0; i < params.table_params().hash_func_count; i++)
    {
        result.emplace_back(params.table_params().table_size, kuku::make_item(i, 0));
    }

    return result;
}

/**
Computes all cuckoo hash table locations for a given item.
*/
inline std::unordered_set<kuku::location_type> allLocations(
    const std::vector<kuku::LocFunc>& hash_funcs, const apsi::HashedItem& item)
{
    std::unordered_set<kuku::location_type> result;
    for (auto& hf : hash_funcs)
    {
        result.emplace(hf(item.get_as<kuku::item_type>().front()));
    }

    return result;
}

/**
Compute the label size in multiples of item-size chunks.
*/
inline size_t computeLabelSize(size_t label_byte_count, const apsi::PSIParams& params)
{
    return (label_byte_count * 8 + params.item_bit_count() - 1) / params.item_bit_count();
}

/**
Unpacks a cuckoo idx into its bin and bundle indices
*/
inline std::pair<size_t, size_t> unpackCuckooIdx(size_t cuckoo_idx, size_t bins_per_bundle)
{
    // Recall that bin indices are relative to the bundle index. That is, the
    // first bin index of a bundle at bundle index 5 is 0. A cuckoo index is
    // similar, except it is not relative to the bundle index. It just keeps
    // counting past bundle boundaries. So in order to get the bin index from the
    // cuckoo index, just compute cuckoo_idx (mod bins_per_bundle).
    size_t bin_idx = cuckoo_idx % bins_per_bundle;

    // Compute which bundle index this cuckoo index belongs to
    size_t bundle_idx = (cuckoo_idx - bin_idx) / bins_per_bundle;

    return {bin_idx, bundle_idx};
}

/**
Compute oprf hash for items and encrypt labels
*/
inline std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>> preprocessItemsAndLabels(
    ppc::io::DataBatch::Ptr _items, ppc::io::DataBatch::Ptr _labels, size_t _labelByteCount,
    size_t _nonceByteCount, crypto::OprfServer::Ptr _oprfServer)
{
    // do oprf
    size_t dataSize = _items->size();
    std::vector<bcos::bytes> oprfOut;
    oprfOut.reserve(dataSize);
    oprfOut.resize(dataSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            oprfOut[i] = _oprfServer->fullEvaluate(_items->getBytes(i));
        }
    });

    std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>> results(dataSize);
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); ++i)
        {
            // the left half is item hash
            apsi::HashedItem hashedItem;
            std::memcpy(hashedItem.value().data(), oprfOut[i].data(), hashedItem.value().size());

            // the right half is key for label encryption
            apsi::LabelKey key;
            std::memcpy(key.data(), oprfOut[i].data() + hashedItem.value().size(), key.size());

            // encrypt labels
            apsi::EncryptedLabel encryptedLabel =
                encrypt_label(_labels->getBytes(i), key, _labelByteCount, _nonceByteCount);

            results[i] = std::make_pair(hashedItem, encryptedLabel);
        }
    });

    return results;
}

inline std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>> preprocessItemsAndLabels(
    const std::vector<std::pair<apsi::Item, apsi::Label>>& _pairs, size_t _labelByteCount,
    size_t _nonceByteCount, crypto::OprfServer::Ptr _oprfServer)
{
    // do oprf
    size_t dataSize = _pairs.size();
    std::vector<bcos::bytes> oprfOut;
    oprfOut.reserve(dataSize);
    oprfOut.resize(dataSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            oprfOut[i] = _oprfServer->fullEvaluate(
                bcos::bytes(_pairs[i].first.value().begin(), _pairs[i].first.value().end()));
        }
    });

    std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>> results(dataSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); ++i)
        {
            // the left half is item hash
            apsi::HashedItem hashedItem;
            std::memcpy(hashedItem.value().data(), oprfOut[i].data(), hashedItem.value().size());

            // the right half is key for label encryption
            apsi::LabelKey key;
            std::memcpy(key.data(), oprfOut[i].data() + hashedItem.value().size(), key.size());

            // encrypt labels
            apsi::EncryptedLabel encryptedLabel =
                encrypt_label(_pairs[i].second, key, _labelByteCount, _nonceByteCount);

            results[i] = std::make_pair(hashedItem, encryptedLabel);
        }
    });

    return results;
}

/**
Compute oprf hash for items
*/
inline std::vector<apsi::HashedItem> preprocessItems(
    const std::vector<apsi::Item>& _items, crypto::OprfServer::Ptr _oprfServer)
{
    // do oprf
    size_t dataSize = _items.size();
    std::vector<bcos::bytes> oprfOut;
    oprfOut.reserve(dataSize);
    oprfOut.resize(dataSize);
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            std::string itemStr(_items[i].value().size(), 0);
            std::memcpy(itemStr.data(), _items[i].value().data(), _items[i].value().size());
            oprfOut[i] = _oprfServer->fullEvaluate(itemStr);
        }
    });

    std::vector<apsi::HashedItem> results;
    for (auto& hash : oprfOut)
    {
        // truncate hash
        apsi::Item::value_type value{};
        std::memcpy(value.data(), hash.data(), value.size());
        results.emplace_back(value);
    }

    return results;
}

/**
Converts each given Item-Label pair in between the given iterators into its
algebraic form, i.e., a sequence of felt-felt pairs. Also computes each Item's
cuckoo index.
*/
std::vector<std::pair<apsi::util::AlgItemLabel, size_t>> transformLabeledData(
    const std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>>::const_iterator begin,
    const std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>>::const_iterator end,
    const apsi::PSIParams& params)
{
    LABELED_PSI_LOG(DEBUG) << LOG_DESC("start transforming labeled data")
                           << LOG_KV("count", distance(begin, end));

    // Some variables we'll need
    size_t bins_per_item = params.item_params().felts_per_item;
    size_t item_bit_count = params.item_bit_count();

    // Set up Kuku hash functions
    auto hash_funcs = hashFunctions(params);

    // Calculate the cuckoo indices for each item. Store every pair of
    // (item-label, cuckoo_idx) in a vector. Later, we're gonna sort this vector
    // by cuckoo_idx and use the result to parallelize the work of inserting the
    // items into BinBundles.
    std::vector<std::pair<apsi::util::AlgItemLabel, size_t>> data_with_indices;
    for (auto it = begin; it != end; it++)
    {
        const std::pair<apsi::HashedItem, apsi::EncryptedLabel>& item_label_pair = *it;

        // Serialize the data into field elements
        const apsi::HashedItem& item = item_label_pair.first;
        const apsi::EncryptedLabel& label = item_label_pair.second;
        apsi::util::AlgItemLabel alg_item_label = algebraize_item_label(
            item, label, item_bit_count, params.seal_params().plain_modulus());

        // Get the cuckoo table locations for this item and add to data_with_indices
        for (auto location : allLocations(hash_funcs, item))
        {
            // The current hash value is an index into a table of Items. In reality
            // our BinBundles are tables of bins, which contain chunks of items. How
            // many chunks? bins_per_item many chunks
            size_t bin_idx = location * bins_per_item;

            // Store the data along with its index
            data_with_indices.emplace_back(alg_item_label, bin_idx);
        }
    }

    LABELED_PSI_LOG(DEBUG) << LOG_DESC("finished transforming labeled data")
                           << LOG_KV("count", distance(begin, end));

    return data_with_indices;
}

/**
Converts each given Item into its algebraic form, i.e., a sequence of
felt-monostate pairs. Also computes each Item's cuckoo index.
*/
std::vector<std::pair<apsi::util::AlgItem, size_t>> transformUnlabeledData(
    const std::vector<apsi::HashedItem>::const_iterator begin,
    const std::vector<apsi::HashedItem>::const_iterator end, const apsi::PSIParams& params)
{
    LABELED_PSI_LOG(DEBUG) << LOG_DESC("start transforming unlabeled data")
                           << LOG_KV("count", distance(begin, end));

    // Some variables we'll need
    size_t bins_per_item = params.item_params().felts_per_item;
    size_t item_bit_count = params.item_bit_count();

    // Set up Kuku hash functions
    auto hash_funcs = hashFunctions(params);

    // Calculate the cuckoo indices for each item. Store every pair of
    // (item-label, cuckoo_idx) in a vector. Later, we're gonna sort this vector
    // by cuckoo_idx and use the result to parallelize the work of inserting the
    // items into BinBundles.
    std::vector<std::pair<apsi::util::AlgItem, size_t>> data_with_indices;
    for (auto it = begin; it != end; it++)
    {
        const apsi::HashedItem& item = *it;

        // Serialize the data into field elements
        apsi::util::AlgItem alg_item =
            algebraize_item(item, item_bit_count, params.seal_params().plain_modulus());

        // Get the cuckoo table locations for this item and add to data_with_indices
        for (auto location : allLocations(hash_funcs, item))
        {
            // The current hash value is an index into a table of Items. In reality
            // our BinBundles are tables of bins, which contain chunks of items. How
            // many chunks? bins_per_item many chunks
            size_t bin_idx = location * bins_per_item;

            // Store the data along with its index
            data_with_indices.emplace_back(std::make_pair(alg_item, bin_idx));
        }
    }

    LABELED_PSI_LOG(DEBUG) << LOG_DESC("finished transforming unlabeled data")
                           << LOG_KV("count", distance(begin, end));

    return data_with_indices;
}

/**
Converts given Item into its algebraic form, i.e., a sequence of felt-monostate
pairs. Also computes the Item's cuckoo index.
*/
std::vector<std::pair<apsi::util::AlgItem, size_t>> transformUnlabeledData(
    const apsi::HashedItem& item, const apsi::PSIParams& params)
{
    std::vector<apsi::HashedItem> item_singleton{item};
    return transformUnlabeledData(item_singleton.begin(), item_singleton.end(), params);
}

/**
Inserts the given items and corresponding labels into bin_bundles at their
respective cuckoo indices. It will only insert the data with bundle index in the
half-open range range indicated by work_range. If inserting into a BinBundle
would make the number of items in a bin larger than max_bin_size, this function
will create and insert a new BinBundle. If overwrite is set, this will overwrite
the labels if it finds an AlgItemLabel that matches the input perfectly.
*/
template <typename T>
void insertOrAssignWorker(std::vector<BinBundle>& bin_bundles,
    const std::vector<std::pair<T, size_t>>& data_with_indices,
    const apsi::CryptoContext& crypto_context, uint32_t bundle_index, size_t label_size,
    const apsi::PSIParams& _params, bool overwrite, bool compressed)
{
    LABELED_PSI_LOG(DEBUG) << LOG_DESC("start insert-or-assign worker")
                           << LOG_KV("bundleIndex", bundle_index) << LOG_KV("overwrite", overwrite);

    // Iteratively insert each item-label pair at the given cuckoo index
    for (auto& data_with_idx : data_with_indices)
    {
        const T& data = data_with_idx.first;

        // Get the bundle index
        size_t cuckoo_idx = data_with_idx.second;
        size_t bin_idx, bundle_idx;
        std::tie(bin_idx, bundle_idx) = unpackCuckooIdx(cuckoo_idx, _params.bins_per_bundle());

        // If the bundle_idx isn't in the prescribed range, don't try to insert this
        // data
        if (bundle_idx != bundle_index)
        {
            // Dealing with this bundle index is not our job
            continue;
        }

        // Try to insert or overwrite these field elements in an existing BinBundle
        // at this bundle index. Keep track of whether we succeed or not.
        bool written = false;
        for (auto bundle_it = bin_bundles.rbegin(); bundle_it != bin_bundles.rend(); bundle_it++)
        {
            // If we're supposed to overwrite, try to overwrite. One of these
            // BinBundles has to have the data we're trying to overwrite.
            if (overwrite)
            {
                // If we successfully overwrote, we're done with this bundle
                written = bundle_it->try_multi_overwrite(data, bin_idx);
                if (written)
                {
                    break;
                }
            }

            // Do a dry-run insertion and see if the new largest bin size in the range
            // exceeds the limit
            int32_t new_largest_bin_size = bundle_it->multi_insert_dry_run(data, bin_idx);

            // Check if inserting would violate the max bin size constraint
            if (new_largest_bin_size > 0 && seal::util::safe_cast<size_t>(new_largest_bin_size) <
                                                _params.table_params().max_items_per_bin)
            {
                // All good
                bundle_it->multi_insert_for_real(data, bin_idx);
                written = true;
                break;
            }
        }

        // We tried to overwrite an item that doesn't exist. This should never
        // happen
        if (overwrite && !written)
        {
            BOOST_THROW_EXCEPTION(OverwriteItemException()
                                  << bcos::errinfo_comment("Tried to overwrite non-existent item"));
        }

        // If we had conflicts everywhere when trying to insert, then we need to
        // make a new BinBundle and insert the data there
        if (!written)
        {
            // Make a fresh BinBundle and insert
            BinBundle new_bin_bundle(crypto_context, label_size,
                _params.table_params().max_items_per_bin, _params.query_params().ps_low_degree,
                _params.bins_per_bundle(), compressed, false);
            int res = new_bin_bundle.multi_insert_for_real(data, bin_idx);

            // If even that failed, I don't know what could've happened
            if (res < 0)
            {
                BOOST_THROW_EXCEPTION(InsertItemException() << bcos::errinfo_comment(
                                          "Failed to insert item into a new BinBundle with index " +
                                          std::to_string(bundle_idx)));
            }

            // Push a new BinBundle to the set of BinBundles at this bundle index
            bin_bundles.push_back(std::move(new_bin_bundle));
        }
    }

    LABELED_PSI_LOG(DEBUG) << LOG_DESC("finished insert-or-assign worker")
                           << LOG_KV("bundleIndex", bundle_index) << LOG_KV("overwrite", overwrite);
}


/**
Takes algebraized data to be inserted, splits it up, and distributes it so that
thread_count many threads can all insert in parallel. If overwrite is set, this
will overwrite the labels if it finds an AlgItemLabel that matches the input
perfectly.
*/
template <typename T>
void dispatchInsertOrAssign(std::vector<std::vector<BinBundle>>& bin_bundles,
    const std::vector<std::pair<T, size_t>>& data_with_indices,
    const apsi::CryptoContext& crypto_context, size_t label_size, const apsi::PSIParams& _params,
    bool overwrite, bool compressed)
{
    // Collect the bundle indices and partition them into thread_count many
    // partitions. By some uniformity assumption, the number of things to insert
    // per partition should be roughly the same. Note that the contents of
    // bundle_indices is always sorted (increasing order).
    std::set<size_t> bundle_indices_set;
    for (auto& data_with_idx : data_with_indices)
    {
        size_t cuckoo_idx = data_with_idx.second;
        size_t bin_idx, bundle_idx;
        std::tie(bin_idx, bundle_idx) = unpackCuckooIdx(cuckoo_idx, _params.bins_per_bundle());
        bundle_indices_set.insert(bundle_idx);
    }

    std::vector<size_t> bundle_indices;
    bundle_indices.assign(bundle_indices_set.begin(), bundle_indices_set.end());

    tbb::parallel_for(
        tbb::blocked_range<size_t>(0U, bundle_indices.size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); ++i)
            {
                auto bundleIndex = static_cast<uint32_t>(bundle_indices[i]);
                insertOrAssignWorker(bin_bundles[bundleIndex], data_with_indices, crypto_context,
                    bundleIndex, label_size, _params, overwrite, compressed);
            }
        });

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished all insert-or-assign worker tasks");
}


/**
Removes the given items and corresponding labels from bin_bundles at their
respective cuckoo indices.
*/
void removeWorker(std::vector<BinBundle>& bin_bundles,
    const std::vector<std::pair<apsi::util::AlgItem, size_t>>& data_with_indices,
    uint32_t bundle_index, uint32_t bins_per_bundle)
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("start remove worker") << LOG_KV("bundleIndex", bundle_index);


    // Iteratively remove each item-label pair at the given cuckoo index
    for (auto& data_with_idx : data_with_indices)
    {
        // Get the bundle index
        size_t cuckoo_idx = data_with_idx.second;
        size_t bin_idx, bundle_idx;
        std::tie(bin_idx, bundle_idx) = unpackCuckooIdx(cuckoo_idx, bins_per_bundle);

        // If the bundle_idx isn't in the prescribed range, don't try to remove this
        // data
        if (bundle_idx != bundle_index)
        {
            // Dealing with this bundle index is not our job
            continue;
        }

        // Try to remove these field elements from an existing BinBundle at this
        // bundle index. Keep track of whether we succeed or not.
        bool removed = false;
        for (BinBundle& bundle : bin_bundles)
        {
            // If we successfully removed, we're done with this bundle
            removed = bundle.try_multi_remove(data_with_idx.first, bin_idx);
            if (removed)
            {
                break;
            }
        }

        // We may have produced some empty BinBundles so just remove them all
        auto rem_it = std::remove_if(
            bin_bundles.begin(), bin_bundles.end(), [](auto& bundle) { return bundle.empty(); });
        bin_bundles.erase(rem_it, bin_bundles.end());

        // We tried to remove an item that doesn't exist. This should never happen
        if (!removed)
        {
            BOOST_THROW_EXCEPTION(RemoveItemException() << bcos::errinfo_comment(
                                      "Failed to remove item that do not exist, bundle index: " +
                                      std::to_string(bundle_idx)));
        }
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished removing items")
                          << LOG_KV("bundleIndex", bundle_index);
}

/**
Takes algebraized data to be removed, splits it up, and distributes it so that
thread_count many threads can all remove in parallel.
*/
void dispatchRemove(std::vector<std::vector<BinBundle>>& bin_bundles,
    const std::vector<std::pair<apsi::util::AlgItem, size_t>>& data_with_indices,
    uint32_t bins_per_bundle)
{
    // Collect the bundle indices and partition them into thread_count many
    // partitions. By some uniformity assumption, the number of things to remove
    // per partition should be roughly the same. Note that the contents of
    // bundle_indices is always sorted (increasing order).
    std::set<size_t> bundle_indices_set;
    for (auto& data_with_idx : data_with_indices)
    {
        size_t cuckoo_idx = data_with_idx.second;
        size_t bin_idx, bundle_idx;
        std::tie(bin_idx, bundle_idx) = unpackCuckooIdx(cuckoo_idx, bins_per_bundle);
        bundle_indices_set.insert(bundle_idx);
    }

    std::vector<size_t> bundle_indices;
    bundle_indices.assign(bundle_indices_set.begin(), bundle_indices_set.end());

    tbb::parallel_for(
        tbb::blocked_range<size_t>(0U, bundle_indices.size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); ++i)
            {
                auto bundleIndex = static_cast<uint32_t>(bundle_indices[i]);
                removeWorker(
                    bin_bundles[bundleIndex], data_with_indices, bundleIndex, bins_per_bundle);
            }
        });

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished all remove worker tasks");
}

/**
Returns a set of DB cache references corresponding to the bundles in the given
set
*/
std::vector<std::reference_wrapper<const BinBundleCache>> collectCaches(
    std::vector<BinBundle>* bin_bundles)
{
    std::vector<std::reference_wrapper<const BinBundleCache>> result;
    for (const auto& bundle : (*bin_bundles))
    {
        result.emplace_back(std::cref(bundle.get_cache()));
    }

    return result;
}

SenderDB::SenderDB(const apsi::PSIParams& params, crypto::OprfServer::Ptr _oprfServer,
    std::size_t label_byte_count, std::size_t nonce_byte_count, bool compressed)
  : params_(params),
    crypto_context_(params_),
    label_byte_count_(label_byte_count),
    nonce_byte_count_(nonce_byte_count),
    item_count_(0),
    compressed_(compressed),
    m_oprfServer(std::move(_oprfServer))
{
    // The labels cannot be more than 1 KB.
    if (label_byte_count_ > MAX_LABEL_BYTE)
    {
        BOOST_THROW_EXCEPTION(LabelExceedsSizeException() << bcos::errinfo_comment(
                                  "Requested label byte count exceeds the maximum (1024), count: " +
                                  std::to_string(label_byte_count_)));
    }

    if (nonce_byte_count_ > apsi::max_nonce_byte_count)
    {
        BOOST_THROW_EXCEPTION(NonceExceedsSizeException() << bcos::errinfo_comment(
                                  "Requested nonce byte count exceeds the maximum (16), count: " +
                                  std::to_string(nonce_byte_count_)));
    }

    // Set the evaluator. This will be used for BatchedPlaintextPolyn::eval.
    crypto_context_.set_evaluator();

    // Reset the SenderDB data structures
    clear();
}

SenderDB::SenderDB(SenderDB&& source)
  : params_(source.params_),
    crypto_context_(source.crypto_context_),
    label_byte_count_(source.label_byte_count_),
    nonce_byte_count_(source.nonce_byte_count_),
    item_count_(source.item_count_),
    compressed_(source.compressed_),
    stripped_(source.stripped_),
    m_oprfServer(std::move(source.m_oprfServer))
{
    // Lock the source before moving stuff over
    auto lock = source.getWriterLock();

    hashed_items_ = std::move(source.hashed_items_);
    bin_bundles_ = std::move(source.bin_bundles_);

    // Reset the source data structures
    source.clearInternal();
}

SenderDB& SenderDB::operator=(SenderDB&& source)
{
    // Do nothing if moving to self
    if (&source == this)
    {
        return *this;
    }

    // Lock the current SenderDB
    auto this_lock = getWriterLock();

    params_ = source.params_;
    crypto_context_ = source.crypto_context_;
    label_byte_count_ = source.label_byte_count_;
    nonce_byte_count_ = source.nonce_byte_count_;
    item_count_ = source.item_count_;
    compressed_ = source.compressed_;
    stripped_ = source.stripped_;
    m_oprfServer = std::move(source.m_oprfServer);

    // Lock the source before moving stuff over
    auto source_lock = source.getWriterLock();

    hashed_items_ = std::move(source.hashed_items_);
    bin_bundles_ = std::move(source.bin_bundles_);

    // Reset the source data structures
    source.clearInternal();

    return *this;
}

size_t SenderDB::getBinBundleCount(uint32_t bundle_idx) const
{
    // Lock the database for reading
    auto lock = getReaderLock();

    return bin_bundles_.at(seal::util::safe_cast<size_t>(bundle_idx)).size();
}

size_t SenderDB::getBinBundleCount() const
{
    // Lock the database for reading
    auto lock = getReaderLock();

    // Compute the total number of BinBundles
    return std::accumulate(bin_bundles_.cbegin(), bin_bundles_.cend(), size_t(0),
        [&](auto a, auto& b) { return a + b.size(); });
}

double SenderDB::getPackingRate() const
{
    // Lock the database for reading
    auto lock = getReaderLock();

    uint64_t item_count = seal::util::mul_safe(static_cast<uint64_t>(getItemCount()),
        static_cast<uint64_t>(params_.table_params().hash_func_count));
    uint64_t max_item_count = seal::util::mul_safe(static_cast<uint64_t>(getBinBundleCount()),
        static_cast<uint64_t>(params_.items_per_bundle()),
        static_cast<uint64_t>(params_.table_params().max_items_per_bin));

    return max_item_count ? static_cast<double>(item_count) / static_cast<double>(max_item_count) :
                            0.0;
}

void SenderDB::clearInternal()
{
    // Assume the SenderDB is already locked for writing

    // Clear the set of inserted items
    hashed_items_.clear();
    item_count_ = 0;

    // Clear the BinBundles
    bin_bundles_.clear();
    bin_bundles_.resize(params_.bundle_idx_count());

    // Reset the stripped_ flag
    stripped_ = false;
}

void SenderDB::clear()
{
    // Lock the database for writing
    auto lock = getWriterLock();
    clearInternal();
}

void SenderDB::generateCaches()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("start generating bin bundle caches");

    for (auto& bundle_idx : bin_bundles_)
    {
        for (auto& bb : bundle_idx)
        {
            bb.regen_cache();
        }
    }
    LABELED_PSI_LOG(INFO) << LOG_DESC("finished generating bin bundle caches");
}

std::vector<std::reference_wrapper<const BinBundleCache>> SenderDB::getCacheAt(uint32_t bundle_idx)
{
    return collectCaches(&(bin_bundles_.at(seal::util::safe_cast<size_t>(bundle_idx))));
}

void SenderDB::strip()
{
    // Lock the database for writing
    auto lock = getWriterLock();

    stripped_ = true;

    hashed_items_.clear();

    uint32_t bundleSize = bin_bundles_.size();

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, bundleSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); ++i)
        {
            for (auto& bb : bin_bundles_[i])
            {
                bb.strip();
            }
        }
    });

    LABELED_PSI_LOG(INFO) << LOG_DESC("SenderDB has been stripped");
}


void SenderDB::insertOrAssign(const std::vector<std::pair<apsi::Item, apsi::Label>>& data)
{
    if (stripped_)
    {
        BOOST_THROW_EXCEPTION(InsertItemException() << bcos::errinfo_comment(
                                  "Cannot insert data to a stripped SenderDB"));
    }
    if (!isLabeled())
    {
        BOOST_THROW_EXCEPTION(
            InsertItemException() << bcos::errinfo_comment(
                "Attempted to insert labeled data but this is an unlabeled SenderDB"));
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("start inserting items and labels in SenderDB")
                          << LOG_KV("count", data.size());

    auto hashedData =
        preprocessItemsAndLabels(data, label_byte_count_, nonce_byte_count_, m_oprfServer);

    doInsertOrAssign(hashedData);

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished inserting items and labels in SenderDB");
}


void SenderDB::insertOrAssign(ppc::io::DataBatch::Ptr _items, ppc::io::DataBatch::Ptr _labels)
{
    if (stripped_)
    {
        BOOST_THROW_EXCEPTION(InsertItemException() << bcos::errinfo_comment(
                                  "Cannot insert data to a stripped SenderDB"));
    }
    if (!isLabeled())
    {
        BOOST_THROW_EXCEPTION(
            InsertItemException() << bcos::errinfo_comment(
                "Attempted to insert labeled data but this is an unlabeled SenderDB"));
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("start inserting items and labels in SenderDB")
                          << LOG_KV("count", _items->size());

    auto hashedData = preprocessItemsAndLabels(
        std::move(_items), std::move(_labels), label_byte_count_, nonce_byte_count_, m_oprfServer);

    doInsertOrAssign(hashedData);

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished inserting items and labels in SenderDB");
}


void SenderDB::doInsertOrAssign(
    std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>>& _hashedData)
{
    // Lock the database for writing
    auto lock = getWriterLock();

    std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>> newData;
    std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>> reWriteData;

    for (auto itemLabelPair : _hashedData)
    {
        if (hashed_items_.find(itemLabelPair.first) == hashed_items_.end())
        {
            hashed_items_.insert(itemLabelPair.first);
            item_count_++;
            newData.emplace_back(std::move(itemLabelPair));
        }
        else
        {
            reWriteData.emplace_back(std::move(itemLabelPair));
        }
    }

    // Compute the label size; this ceil(effective_label_bit_count / item_bit_count)
    size_t labelSize = computeLabelSize(nonce_byte_count_ + label_byte_count_, params_);

    auto newItemCount = newData.size();
    auto existingItemCount = reWriteData.size();

    // Dispatch the insertion, first for the new data, then for the data we're gonna overwrite

    if (existingItemCount)
    {
        LABELED_PSI_LOG(INFO) << LOG_DESC("found existing items to replace in SenderDB")
                              << LOG_KV("count", existingItemCount);

        // Break the data into field element representation. Also compute the items'
        // cuckoo indices.
        std::vector<std::pair<apsi::util::AlgItemLabel, size_t>> data_with_indices =
            transformLabeledData(reWriteData.begin(), reWriteData.end(), params_);

        dispatchInsertOrAssign(bin_bundles_, data_with_indices, crypto_context_, labelSize, params_,
            /* overwrite items */ true, compressed_);
    }
    if (newItemCount)
    {
        LABELED_PSI_LOG(INFO) << LOG_DESC("found new items to replace in SenderDB")
                              << LOG_KV("count", newItemCount);

        // Process and add the new data. Break the data into field element
        // representation. Also compute the items' cuckoo indices.
        std::vector<std::pair<apsi::util::AlgItemLabel, size_t>> data_with_indices =
            transformLabeledData(newData.begin(), newData.end(), params_);

        dispatchInsertOrAssign(bin_bundles_, data_with_indices, crypto_context_, labelSize, params_,
            /* don't overwrite items */ false, compressed_);
    }

    // Generate the BinBundle caches
    generateCaches();
}

void SenderDB::insertOrAssign(const std::vector<apsi::Item>& data)
{
    if (stripped_)
    {
        BOOST_THROW_EXCEPTION(InsertItemException() << bcos::errinfo_comment(
                                  "Cannot insert item to a stripped SenderDB"));
    }
    if (isLabeled())
    {
        BOOST_THROW_EXCEPTION(InsertItemException() << bcos::errinfo_comment(
                                  "Attempted to insert item data but this is an labeled SenderDB"));
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("start inserting items in SenderDB")
                          << LOG_KV("count", data.size());

    std::vector<apsi::HashedItem> hashedData = preprocessItems(data, m_oprfServer);

    // Lock the database for writing
    auto lock = getWriterLock();

    // We are not going to insert items that already appear in the database.
    auto newDataEnd = std::remove_if(hashedData.begin(), hashedData.end(), [&](const auto& item) {
        bool found = hashed_items_.find(item) != hashed_items_.end();
        if (!found)
        {
            // Add to hashed_items_ already at this point!
            hashed_items_.insert(item);
            item_count_++;
        }

        // Remove those that were found
        return found;
    });

    // Erase the previously existing items from hashed_data; in unlabeled case
    // there is nothing to do
    hashedData.erase(newDataEnd, hashedData.end());

    LABELED_PSI_LOG(INFO) << LOG_DESC("found new items to replace in SenderDB")
                          << LOG_KV("count", hashedData.size());

    // Break the new data down into its field element representation. Also compute
    // the items' cuckoo indices.
    std::vector<std::pair<apsi::util::AlgItem, size_t>> data_with_indices =
        transformUnlabeledData(hashedData.begin(), hashedData.end(), params_);

    // Dispatch the insertion
    dispatchInsertOrAssign(bin_bundles_, data_with_indices, crypto_context_,
        /* label size */ 0, params_, /* don't overwrite items */ false, compressed_);

    // Generate the BinBundle caches
    generateCaches();

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished inserting items in SenderDB");
}

void SenderDB::remove(const std::vector<apsi::Item>& data)
{
    if (stripped_)
    {
        BOOST_THROW_EXCEPTION(RemoveItemException() << bcos::errinfo_comment(
                                  "Cannot remove data from a stripped SenderDB"));
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("start removing items in SenderDB")
                          << LOG_KV("count", data.size());

    std::vector<apsi::HashedItem> hashedData = preprocessItems(data, m_oprfServer);

    // Lock the database for writing
    auto lock = getWriterLock();

    // Remove items that do not exist in the database.
    std::remove_if(hashedData.begin(), hashedData.end(), [&](const auto& item) {
        bool found = hashed_items_.find(item) != hashed_items_.end();
        if (found)
        {
            // Remove from hashed_items_ already at this point!
            hashed_items_.erase(item);
            item_count_--;
        }

        // Remove those that were not found
        return !found;
    });

    // Break the data down into its field element representation. Also compute the
    // items' cuckoo indices.
    std::vector<std::pair<apsi::util::AlgItem, size_t>> data_with_indices =
        transformUnlabeledData(hashedData.begin(), hashedData.end(), params_);

    // Dispatch the removal
    uint32_t bins_per_bundle = params_.bins_per_bundle();
    dispatchRemove(bin_bundles_, data_with_indices, bins_per_bundle);

    // Generate the BinBundle caches
    generateCaches();

    LABELED_PSI_LOG(INFO) << LOG_DESC("finished removing items in SenderDB");
}

bool SenderDB::hasItem(const apsi::Item& item) const
{
    if (stripped_)
    {
        BOOST_THROW_EXCEPTION(
            RetrieveItemException() << bcos::errinfo_comment(
                "Cannot retrieve the presence of an item from a stripped SenderDB"));
    }

    std::string itemStr(item.value().size(), 0);
    std::memcpy(&itemStr[0], item.value().data(), item.value().size());

    bcos::bytes oprfOut = m_oprfServer->fullEvaluate(itemStr);

    apsi::HashedItem hashedItem;
    std::memcpy(hashedItem.value().data(), oprfOut.data(), hashedItem.value().size());

    // Lock the database for reading
    auto lock = getReaderLock();

    return hashed_items_.find(hashedItem) != hashed_items_.end();
}

apsi::Label SenderDB::getLabel(const apsi::Item& item) const
{
    if (stripped_)
    {
        BOOST_THROW_EXCEPTION(
            RetrieveLabelException() << bcos::errinfo_comment(
                "Cannot retrieve the presence of a label from a stripped SenderDB"));
    }
    if (!isLabeled())
    {
        BOOST_THROW_EXCEPTION(
            RetrieveLabelException() << bcos::errinfo_comment(
                "Attempted to retrieve a label but this is an unlabeled SenderDB"));
    }

    std::string itemStr(item.value().size(), 0);
    std::memcpy(&itemStr[0], item.value().data(), item.value().size());

    bcos::bytes oprfOut = m_oprfServer->fullEvaluate(itemStr);

    apsi::HashedItem hashedItem;
    std::memcpy(hashedItem.value().data(), oprfOut.data(), hashedItem.value().size());

    // Lock the database for reading
    auto lock = getReaderLock();

    // Check if this item is in the DB. If not, throw an exception
    if (hashed_items_.find(hashedItem) == hashed_items_.end())
    {
        BOOST_THROW_EXCEPTION(RetrieveLabelException() << bcos::errinfo_comment(
                                  "Cannot retrieve label for an item that is not in the SenderDB"));
    }

    uint32_t bins_per_bundle = params_.bins_per_bundle();

    // Preprocess a single element. This algebraizes the item and gives back its
    // field element representation as well as its cuckoo hash. We only read one
    // of the locations because the labels are the same in each location.
    apsi::util::AlgItem alg_item;
    size_t cuckoo_idx;
    std::tie(alg_item, cuckoo_idx) = transformUnlabeledData(hashedItem, params_)[0];

    // Now figure out where to look to get the label
    size_t bin_idx, bundle_idx;
    std::tie(bin_idx, bundle_idx) = unpackCuckooIdx(cuckoo_idx, bins_per_bundle);

    // Retrieve the algebraic labels from one of the BinBundles at this index
    const std::vector<BinBundle>& bundle_set = bin_bundles_[bundle_idx];
    std::vector<felt_t> alg_label;
    bool got_labels = false;
    for (const BinBundle& bundle : bundle_set)
    {
        // Try to retrieve the contiguous labels from this BinBundle
        if (bundle.try_get_multi_label(alg_item, bin_idx, alg_label))
        {
            got_labels = true;
            break;
        }
    }

    // It shouldn't be possible to have items in your set but be unable to
    // retrieve the associated label. Throw an exception because something is
    // terribly wrong.
    if (!got_labels)
    {
        BOOST_THROW_EXCEPTION(
            RetrieveLabelException() << bcos::errinfo_comment(
                "Failed to retrieve label for an item that was supposed to be in the SenderDB"));
    }

    // All good. Now just reconstruct the big label from its split-up parts
    apsi::EncryptedLabel encrypted_label = dealgebraize_label(alg_label,
        alg_label.size() * static_cast<size_t>(params_.item_bit_count_per_felt()),
        params_.seal_params().plain_modulus());

    // Resize down to the effective byte count
    encrypted_label.resize(nonce_byte_count_ + label_byte_count_);

    apsi::LabelKey key;
    std::memcpy(key.data(), oprfOut.data() + hashedItem.value().size(), key.size());

    // Decrypt the label
    return decrypt_label(encrypted_label, key, nonce_byte_count_);
}

/**
Writes the SenderDB bytes
*/
void SenderDB::saveToBytes(bcos::bytes& _out) const
{
    // Lock the database for reading
    auto lock = getReaderLock();
    LABELED_PSI_LOG(INFO) << LOG_DESC("start saving SenderDB");

    ppctars::SenderDB tarsSenderDB;
    for (auto& hashedItems : hashed_items_)
    {
        bcos::bytes item(sizeof(apsi::Item::value_type));
        std::memcpy(item.data(), hashedItems.value().data(), sizeof(apsi::Item::value_type));
        tarsSenderDB.hashedItems.emplace_back(item);
    }
    tarsSenderDB.psiParams = fromPSIParams(params_, 0);
    tarsSenderDB.oprfKey = m_oprfServer->privateKey();
    tarsSenderDB.stripped = stripped_;
    tarsSenderDB.compressed = compressed_;
    tarsSenderDB.itemCount = item_count_;
    tarsSenderDB.nonceByteCount = nonce_byte_count_;
    tarsSenderDB.labelByteCount = label_byte_count_;

    for (auto& binBundles : bin_bundles_)
    {
        std::vector<ppctars::BinBundle> tarsBinBundles;
        tarsBinBundles.reserve(binBundles.size());
        for (auto& binBundle : binBundles)
        {
            tarsBinBundles.emplace_back(binBundle.saveToTarsBinBundle());
        }
        tarsSenderDB.binBundles.emplace_back(tarsBinBundles);
    }

    _out.clear();
    ppctars::serialize::encode(tarsSenderDB, _out);

    LABELED_PSI_LOG(INFO) << LOG_DESC("finish saving SenderDB");
}

/**
Reads the SenderDB from bytes.
*/
SenderDB::Ptr SenderDB::loadFromBytes(crypto::OprfServer::Ptr _oprfServer, const bcos::bytes& _in)
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("start loading SenderDB");

    ppctars::SenderDB tarsSenderDB;
    ppctars::serialize::decode(_in, tarsSenderDB);

    _oprfServer->setPrivateKey(tarsSenderDB.oprfKey);
    SenderDB::Ptr ret =
        std::make_shared<SenderDB>(toPSIParams(tarsSenderDB.psiParams), std::move(_oprfServer),
            tarsSenderDB.labelByteCount, tarsSenderDB.nonceByteCount, tarsSenderDB.compressed);

    ret->stripped_ = tarsSenderDB.stripped;
    ret->item_count_ = tarsSenderDB.itemCount;
    for (auto& tarsHashedItems : tarsSenderDB.hashedItems)
    {
        apsi::HashedItem hashedItem;
        std::memcpy(hashedItem.value().data(), tarsHashedItems.data(), hashedItem.value().size());
        ret->hashed_items_.insert(hashedItem);
    }

    ret->bin_bundles_.clear();
    for (auto& tarsBinBundles : tarsSenderDB.binBundles)
    {
        std::vector<BinBundle> binBundles;
        binBundles.reserve(tarsBinBundles.size());
        for (auto& tarsBinBundle : tarsBinBundles)
        {
            binBundles.emplace_back(
                BinBundle::loadFromTarsBinBundle(ret->crypto_context_, tarsBinBundle));
        }
        ret->bin_bundles_.emplace_back(std::move(binBundles));
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("finish loading SenderDB");
    return ret;
}