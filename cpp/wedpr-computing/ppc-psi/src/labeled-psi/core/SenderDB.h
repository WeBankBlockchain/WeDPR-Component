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
 * @reference https://github.com/microsoft/APSI/sender/sender_db.h
 * @license MIT license
 * @change optimize the implementation of some functions & replace the instance of oprf
 *
 * @file SenderDB.h
 * @author: shawnhe
 * @date 2022-11-7
 *
 */

#pragma once

// STD
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

// GSL
#include <gsl/span>

// APSI
#include <apsi/crypto_context.h>
#include <apsi/item.h>
#include <apsi/psi_params.h>

// SEAL
#include "seal/plaintext.h"
#include "seal/util/locks.h"


#include "ppc-framework/crypto/Oprf.h"

#include "ppc-framework/io/DataBatch.h"

#include "../protocol/Protocol.h"
#include "BinBundle.h"
#include "SenderCache.h"
#include "wedpr-protocol/tars/TarsSerialize.h"

namespace ppc::psi
{
/**
A SenderDB maintains an in-memory representation of the sender's set of items
and labels (in labeled mode). This data is not simply copied into the SenderDB
data structures, but also preprocessed heavily to allow for faster online
computation time. Since inserting a large number of new items into a SenderDB
can take time, it is not recommended to recreate the SenderDB when the database
changes a little bit. Instead, the class supports fast update and deletion
operations that should be preferred: SenderDB::InsertOrAssign and
SenderDB::remove.

The SenderDB constructor allows the label byte count to be specified; unlabeled
mode is activated by setting the label byte count to zero. It is possible to
optionally specify the size of the nonce used in encrypting the labels, but this
is best left to its default value unless the user is absolutely sure of what
they are doing.

The SenderDB requires substantially more memory than the raw data would. Part of
that memory can automatically be compressed when it is not in use; this feature
is enabled by default, and can be disabled when constructing the SenderDB. The
downside of in-memory compression is a performance reduction from decompressing
parts of the data when they are used, and recompressing them if they are
updated.
*/

class SenderDB : public std::enable_shared_from_this<SenderDB>
{
public:
    using Ptr = std::shared_ptr<SenderDB>;
    virtual ~SenderDB() = default;

    /**
    Creates a new SenderDB.
    */
    SenderDB(const apsi::PSIParams& params, crypto::OprfServer::Ptr _oprfServer,
        std::size_t label_byte_count = 0, std::size_t nonce_byte_count = 16,
        bool compressed = true);
    /**
    Creates a new SenderDB by moving from an existing one.
    */
    SenderDB(SenderDB&& source);

    /**
    Moves an existing SenderDB to the current one.
    */
    SenderDB& operator=(SenderDB&& source);

    /**
    Clears the database. Every item and label will be removed. The OPRF key is
    unchanged.
    */
    void clear();

    /**
    Returns whether this is a labeled SenderDB.
    */
    bool isLabeled() const { return 0 != label_byte_count_; }

    /**
    Returns the label byte count. A zero value indicates an unlabeled SenderDB.
    */
    std::size_t getLabelByteCount() const { return label_byte_count_; }

    /**
    Returns the nonce byte count used for encrypting labels.
    */
    std::size_t getNonceByteCount() const { return nonce_byte_count_; }

    /**
    Indicates whether SEAL plaintexts are compressed in memory.
    */
    bool isCompressed() const { return compressed_; }

    /**
    Indicates whether the SenderDB has been stripped of all information not needed
    for serving a query.
    */
    bool isStripped() const { return stripped_; }

    /**
    Strips the SenderDB of all information not needed for serving a query. Returns
    a copy of the OPRF key and clears it from the SenderDB.
    */
    void strip();

    /**
    Inserts the given data into the database. This function can be used only on a
    labeled SenderDB instance. If an item already exists in the database, its
    label is overwritten with the new label.
    */
    void insertOrAssign(const std::vector<std::pair<apsi::Item, apsi::Label>>& data);

    void insertOrAssign(ppc::io::DataBatch::Ptr _items, ppc::io::DataBatch::Ptr _labels);

    // hashedData will be cleared
    void doInsertOrAssign(
        std::vector<std::pair<apsi::HashedItem, apsi::EncryptedLabel>>& _hashedData);

    /**
    Inserts the given (hashed) item-label pair into the database. This function
    can be used only on a labeled SenderDB instance. If the item already exists in
    the database, its label is overwritten with the new label.
    */
    void insertOrAssign(const std::pair<apsi::Item, apsi::Label>& data)
    {
        std::vector<std::pair<apsi::Item, apsi::Label>> data_singleton{data};
        insertOrAssign(data_singleton);
    }

    /**
    Inserts the given data into the database. This function can be used only on an
    unlabeled SenderDB instance.
    */
    void insertOrAssign(const std::vector<apsi::Item>& data);

    /**
    Inserts the given (hashed) item into the database. This function can be used
    only on an unlabeled SenderDB instance.
    */
    void insertOrAssign(const apsi::Item& data)
    {
        std::vector<apsi::Item> data_singleton{data};
        insertOrAssign(data_singleton);
    }

    /**
    Clears the database and inserts the given data. This function can be used only
    on a labeled SenderDB instance.
    */
    void setData(const std::vector<std::pair<apsi::Item, apsi::Label>>& data)
    {
        clear();
        insertOrAssign(data);
    }

    void setData(ppc::io::DataBatch::Ptr _items, ppc::io::DataBatch::Ptr _labels)
    {
        clear();
        insertOrAssign(std::move(_items), std::move(_labels));
    }

    /**
    Clears the database and inserts the given data. This function can be used only
    on an unlabeled SenderDB instance.
    */
    void setData(const std::vector<apsi::Item>& data)
    {
        clear();
        insertOrAssign(data);
    }

    /**
    Removes the given data from the database, using at most thread_count threads.
    */
    void remove(const std::vector<apsi::Item>& data);

    /**
    Removes the given (hashed) item from the database.
    */
    void remove(const apsi::Item& data)
    {
        std::vector<apsi::Item> data_singleton{data};
        remove(data_singleton);
    }

    /**
    Returns whether the given item has been inserted in the SenderDB.
    */
    bool hasItem(const apsi::Item& item) const;

    /**
    Returns the label associated to the given item in the database. Throws
    std::invalid_argument if the item does not appear in the database.
    */
    apsi::Label getLabel(const apsi::Item& item) const;

    /**
    Returns a set of cache references corresponding to the bundles at the given
    bundle index. Even though this function returns a vector, the order has no
    significance. This function is meant for internal use.
    */
    auto getCacheAt(std::uint32_t bundle_idx)
        -> std::vector<std::reference_wrapper<const BinBundleCache>>;

    /**
    Returns a reference to the PSI parameters for this SenderDB.
    */
    const apsi::PSIParams& getParams() const { return params_; }

    /**
    Returns a reference to the CryptoContext for this SenderDB.
    */
    const apsi::CryptoContext& getCryptoContext() const { return crypto_context_; }

    /**
    Returns a reference to the SEALContext for this SenderDB.
    */
    std::shared_ptr<seal::SEALContext> getSealContext() const
    {
        return crypto_context_.seal_context();
    }

    /**
    Returns a reference to a set of item hashes already existing in the SenderDB.
    */
    const std::unordered_set<apsi::HashedItem>& getHashedItems() const { return hashed_items_; }

    /**
    Returns the number of items in this SenderDB.
    */
    size_t getItemCount() const { return item_count_; }

    /**
    Returns the total number of bin bundles at a specific bundle index.
    */
    std::size_t getBinBundleCount(std::uint32_t bundle_idx) const;

    /**
    Returns the total number of bin bundles.
    */
    std::size_t getBinBundleCount() const;

    /**
    Returns how efficiently the SenderDB is packaged. A higher rate indicates
    better performance and a lower communication cost in a query execution.
    */
    double getPackingRate() const;

    /**
    Obtains a scoped lock preventing the SenderDB from being changed.
    */
    seal::util::ReaderLock getReaderLock() const { return db_lock_.acquire_read(); }

    /**
    Writes the SenderDB bytes
    */
    void saveToBytes(bcos::bytes& _out) const;

    /**
    Reads the SenderDB from bytes.
    */
    static SenderDB::Ptr loadFromBytes(crypto::OprfServer::Ptr _oprfServer, const bcos::bytes& _in);

private:
    SenderDB(const SenderDB& copy) = delete;

    seal::util::WriterLock getWriterLock() { return db_lock_.acquire_write(); }

    void clearInternal();

    void generateCaches();

    /**
    The set of all items that have been inserted into the database
    */
    std::unordered_set<apsi::HashedItem> hashed_items_;

    /**
    The PSI parameters define the SEAL parameters, base field, item size, table
    size, etc.
    */
    apsi::PSIParams params_;

    /**
    Necessary for evaluating polynomials of Plaintexts.
    */
    apsi::CryptoContext crypto_context_;

    /**
    A read-write lock to protect the database from modification while in use.
    */
    mutable seal::util::ReaderWriterLocker db_lock_;

    /**
    Indicates the size of the label in bytes. A zero value indicates an unlabeled
    SenderDB.
    */
    std::size_t label_byte_count_;

    /**
    Indicates the number of bytes of the effective label reserved for a randomly
    sampled nonce. The effective label byte count is the sum of label_byte_count
    and nonce_byte_count. The value can range between 0 and 16. If
    label_byte_count is zero, nonce_byte_count has no effect.
    */
    std::size_t nonce_byte_count_;

    /**
    The number of items currently in the SenderDB.
    */
    std::size_t item_count_;

    /**
    Indicates whether SEAL plaintexts are compressed in memory.
    */
    bool compressed_;

    /**
    Indicates whether the SenderDB has been stripped of all information not needed
    for serving a query.
    */
    bool stripped_;

    /**
    All the BinBundles in the database, indexed by bundle index. The set
    (represented by a vector internally) at bundle index i contains all the
    BinBundles with bundle index i.
    */
    std::vector<std::vector<BinBundle>> bin_bundles_;

    crypto::OprfServer::Ptr m_oprfServer;
};  // class SenderDB

}  // namespace ppc::psi
