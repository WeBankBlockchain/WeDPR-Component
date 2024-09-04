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
 * @reference https://github.com/microsoft/APSI/sender/bin_bundle.cpp
 * @license MIT license
 * @change change the implementation of load and save
 *
 * @file Binbundle.cpp
 * @author: shawnhe
 * @date 2022-12-18
 *
 */

#include "BinBundle.h"

// STD
#include <algorithm>
#include <functional>
#include <future>
#include <type_traits>
#include <utility>

// APSI
#include "apsi/bin_bundle.h"
#include "apsi/thread_pool_mgr.h"
#include "apsi/util/interpolate.h"
#include "apsi/util/utils.h"

// SEAL
#include "seal/util/defines.h"

using namespace std;
using namespace seal;
using namespace seal::util;
using namespace apsi;
using namespace apsi::sender::util;
using namespace ppc::psi;

/**
Helper function. Determines if a field element is present in a bin.
*/
bool is_present(const vector<felt_t>& bin, felt_t element)
{
    return bin.end() != find(bin.begin(), bin.end(), element);
}

/**
Helper function. Determines if a field element is present in a bin.
*/
bool is_present(const vector<felt_t>& bin, const CuckooFilter& filter, felt_t element)
{
    // Check if the key is already in the current bin.
    if (filter.contains(element))
    {
        // Perform a linear search to determine true/false positives
        return is_present(bin, element);
    }

    return false;
}

/**
Helper function. Returns an iterator pointing to the given field element in the bin if
found and bin.end() otherwise.
*/
template <typename BinT>
auto get_iterator(BinT& bin, const CuckooFilter& filter, felt_t element)
{
    if (filter.contains(element))
    {
        return find(bin.begin(), bin.end(), element);
    }

    return bin.end();
}

void try_clear_irrelevant_bits(const EncryptionParameters& parms, Ciphertext& ciphertext)
{
    // If the parameter set has only one prime, we can compress the ciphertext by
    // setting low-order bits to zero. This effectively maxes out the noise, but that
    // doesn't matter as long as we don't use quite all noise budget.
    if (parms.coeff_modulus().size() == 1)
    {
        // The number of data bits we need to have left in each ciphertext coefficient
        int compr_coeff_bit_count = parms.plain_modulus().bit_count() +
                                    get_significant_bit_count(parms.poly_modulus_degree())
                                    // Being pretty aggressive here
                                    - 1;

        int coeff_mod_bit_count = parms.coeff_modulus()[0].bit_count();

        // The number of bits to set to zero
        int irrelevant_bit_count = coeff_mod_bit_count - compr_coeff_bit_count;

        // Can compression achieve anything?
        if (irrelevant_bit_count > 0)
        {
            // Mask for zeroing out the irrelevant bits
            uint64_t mask = ~((uint64_t(1) << irrelevant_bit_count) - 1);
            seal_for_each_n(iter(ciphertext), ciphertext.size(), [&](auto&& I) {
                // We only have a single RNS component so dereference once more
                seal_for_each_n(*I, parms.poly_modulus_degree(), [&](auto& J) { J &= mask; });
            });
        }
    }
}

/**
Evaluates the polynomial on the given ciphertext. We don't compute the powers of the input
ciphertext C ourselves. Instead we assume they've been precomputed and accept the powers:
(C, C², C³, ...) as input. The number of powers provided MUST be at least
plaintext_polyn_coeffs_.size()-1.
*/
Ciphertext BatchedPlaintextPolyn::eval(
    const vector<Ciphertext>& ciphertext_powers, MemoryPoolHandle& pool) const
{
#ifdef SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT
    static_assert(false, "SEAL must be built with SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF");
#endif
    // We need to have enough ciphertext powers to evaluate this polynomial
    if (ciphertext_powers.size() < max<size_t>(batched_coeffs.size(), 2))
    {
        throw invalid_argument("not enough ciphertext powers available");
    }

    auto seal_context = crypto_context.seal_context();
    auto evaluator = crypto_context.evaluator();

    // We know now that ciphertext_powers is non-empty so read the parms_id from the first
    // one; they should all be the same.
    const auto& encode_parms_id = ciphertext_powers[1].parms_id();

    // Lowest degree terms are stored in the lowest index positions in vectors.
    // Specifically, ciphertext_powers[1] is the first power of the ciphertext data, but
    // batched_coeffs[0] is the constant coefficient.
    //
    // Because the plaintexts in batched_coeffs can be identically zero, SEAL should be
    // built with SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF. We create a result ciphertext
    // that is identically zero and set its NTT form flag to true so the additions below
    // will work.
    Ciphertext result(pool);
    result.resize(*seal_context, encode_parms_id, 2);
    result.is_ntt_form() = true;
    Ciphertext temp(pool);
    Plaintext coeff(pool);
    for (size_t deg = 1; deg < batched_coeffs.size(); deg++)
    {
        coeff.unsafe_load(*seal_context,
            reinterpret_cast<const seal_byte*>(batched_coeffs[deg].data()),
            batched_coeffs[deg].size());
        evaluator->multiply_plain(ciphertext_powers[deg], coeff, temp, pool);
        evaluator->add_inplace(result, temp);
    }

    // Need to transform back from NTT form before we can add the constant coefficient. The
    // constant coefficient is specifically not in NTT form so this can work.
    evaluator->transform_from_ntt_inplace(result);
    coeff.unsafe_load(*seal_context, reinterpret_cast<const seal_byte*>(batched_coeffs[0].data()),
        batched_coeffs[0].size());
    evaluator->add_plain_inplace(result, coeff);

    // Make the result as small as possible by modulus switching and possibly clearing
    // irrelevant bits.
    while (result.parms_id() != seal_context->last_parms_id())
    {
        evaluator->mod_switch_to_next_inplace(result, pool);
    }
    try_clear_irrelevant_bits(seal_context->last_context_data()->parms(), result);

    return result;
}

/**
Evaluates the polynomial on the given ciphertext using the Paterson-Stockmeyer algorithm,
as long as it requires less computation than the standard evaluation function above.
The algorithm computes h+1 inner polynomials on low powers (C^1 to C^{l-1}).
Each inner polynomial is then multiplied by the corresponding high power.
The parameters l and h are determined according to the degree of the polynomial and the
number of splits in order to minimize the computation complexity.

Evaluated polynomial a_0 + a_1*C + a_2*C^2 + ... + C^degree

Inner polynomials: a_{l*i} + a_{l*i+1}*C + ... + a_{l*i+l-1}*C^{l-1}    (for i=0,...,h-1)
             and: a_{l*h} + a_{l*h+1}*C + ... + a_{l*h+degree%l}*C^{degree%l}  (for i=h)

Low powers:  C^{1}, ..., C^{l-1}
High powers: C^{1*l}, ..., C^{l*h}
*/
Ciphertext BatchedPlaintextPolyn::eval_patstock(const CryptoContext& eval_crypto_context,
    const vector<Ciphertext>& ciphertext_powers, size_t ps_low_degree, MemoryPoolHandle& pool) const
{
#ifdef SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT
    static_assert(false, "SEAL must be built with SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF");
#endif
    // We need to have enough ciphertext powers to evaluate this polynomial
    if (ciphertext_powers.size() < max<size_t>(batched_coeffs.size(), 2))
    {
        throw invalid_argument("not enough ciphertext powers available");
    }

    // This function should not be called when the low-degree is 1
    size_t degree = batched_coeffs.size() - 1;
    if (ps_low_degree <= 1 || ps_low_degree >= degree)
    {
        throw invalid_argument(
            "ps_low_degree must be greater than 1 and less than the "
            "size of batched_coeffs");
    }

    auto seal_context = eval_crypto_context.seal_context();
    auto evaluator = eval_crypto_context.evaluator();
    auto relin_keys = eval_crypto_context.relin_keys();
    bool relinearize = eval_crypto_context.seal_context()->using_keyswitching();

    auto high_powers_parms_id = get_parms_id_for_chain_idx(*crypto_context.seal_context(), 1);

    // This is the number of high-degree powers we have: the first high-degree is
    // ps_low_degree + 1 and the rest are multiples of that up to (but not exceeding) the
    // total degree.
    size_t ps_high_degree = ps_low_degree + 1;
    size_t ps_high_degree_powers = degree / ps_high_degree;

    // Lowest degree terms are stored in the lowest index positions in vectors.
    // Specifically, ciphertext_powers[1] is the first power of the ciphertext data, but
    // batched_coeffs[0] is the constant coefficient.
    //
    // Because the plaintexts in batched_coeffs can be identically zero, SEAL should be
    // built with SEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF. We create a result ciphertext
    // that is identically zero and set its NTT form flag to true so the additions below
    // will work. The ciphertext here will have three components; we relinearize only at the
    // end.
    Ciphertext result(pool);
    result.resize(*seal_context, high_powers_parms_id, 3);
    result.is_ntt_form() = false;

    // Temporary variables
    Ciphertext temp(pool);
    Ciphertext temp_in(pool);
    Plaintext coeff(pool);

    // Calculate polynomial for i=1,...,ps_high_degree_powers-1
    for (size_t i = 1; i < ps_high_degree_powers; i++)
    {
        // Evaluate inner polynomial. The free term is left out and added later on.
        // The evaluation result is stored in temp_in.
        for (size_t j = 1; j < ps_high_degree; j++)
        {
            coeff.unsafe_load(*seal_context,
                reinterpret_cast<const seal_byte*>(batched_coeffs[i * ps_high_degree + j].data()),
                batched_coeffs[i * ps_high_degree + j].size());

            evaluator->multiply_plain(ciphertext_powers[j], coeff, temp, pool);

            if (j == 1)
            {
                temp_in = temp;
            }
            else
            {
                evaluator->add_inplace(temp_in, temp);
            }
        }

        // Transform inner polynomial to coefficient form
        evaluator->transform_from_ntt_inplace(temp_in);
        evaluator->mod_switch_to_inplace(temp_in, high_powers_parms_id);

        // The high powers are already in coefficient form
        evaluator->multiply_inplace(temp_in, ciphertext_powers[i * ps_high_degree], pool);
        evaluator->add_inplace(result, temp_in);
    }

    // Calculate polynomial for i=ps_high_degree_powers.
    // Done separately because here the degree of the inner poly is degree % ps_high_degree.
    // Once again, the free term will only be added later on.
    if (degree % ps_high_degree > 0)
    {
        for (size_t j = 1; j <= degree % ps_high_degree; j++)
        {
            coeff.unsafe_load(*seal_context,
                reinterpret_cast<const seal_byte*>(
                    batched_coeffs[ps_high_degree_powers * ps_high_degree + j].data()),
                batched_coeffs[ps_high_degree_powers * ps_high_degree + j].size());

            evaluator->multiply_plain(ciphertext_powers[j], coeff, temp, pool);

            if (j == 1)
            {
                temp_in = temp;
            }
            else
            {
                evaluator->add_inplace(temp_in, temp);
            }
        }

        // Transform inner polynomial to coefficient form
        evaluator->transform_from_ntt_inplace(temp_in);
        evaluator->mod_switch_to_inplace(temp_in, high_powers_parms_id);

        // The high powers are already in coefficient form
        evaluator->multiply_inplace(
            temp_in, ciphertext_powers[ps_high_degree_powers * ps_high_degree], pool);
        evaluator->add_inplace(result, temp_in);
    }

    // Relinearize sum of ciphertext-ciphertext products if relinearization is supported by
    // the parameters.
    if (relinearize)
    {
        evaluator->relinearize_inplace(result, *relin_keys, pool);
    }

    // Calculate inner polynomial for i=0.
    // Done separately since there is no multiplication with a power of high-degree
    for (size_t j = 1; j < ps_high_degree; j++)
    {
        coeff.unsafe_load(*seal_context,
            reinterpret_cast<const seal_byte*>(batched_coeffs[j].data()), batched_coeffs[j].size());

        evaluator->multiply_plain(ciphertext_powers[j], coeff, temp, pool);
        evaluator->transform_from_ntt_inplace(temp);
        evaluator->mod_switch_to_inplace(temp, high_powers_parms_id);
        evaluator->add_inplace(result, temp);
    }

    // Add the constant coefficients of the inner polynomials multiplied by the respective
    // powers of high-degree
    for (size_t i = 1; i < ps_high_degree_powers + 1; i++)
    {
        coeff.unsafe_load(*seal_context,
            reinterpret_cast<const seal_byte*>(batched_coeffs[i * ps_high_degree].data()),
            batched_coeffs[i * ps_high_degree].size());

        evaluator->multiply_plain(ciphertext_powers[i * ps_high_degree], coeff, temp, pool);
        evaluator->mod_switch_to_inplace(temp, high_powers_parms_id);
        evaluator->add_inplace(result, temp);
    }

    // Add the constant coefficient
    coeff.unsafe_load(*seal_context, reinterpret_cast<const seal_byte*>(batched_coeffs[0].data()),
        batched_coeffs[0].size());

    evaluator->add_plain_inplace(result, coeff);

    // Make the result as small as possible by modulus switching and possibly clearing
    // irrelevant bits.
    while (result.parms_id() != seal_context->last_parms_id())
    {
        evaluator->mod_switch_to_next_inplace(result, pool);
    }
    try_clear_irrelevant_bits(seal_context->last_context_data()->parms(), result);

    return result;
}

/**
Constructs a batched Plaintext polynomial from a list of polynomials. Takes an evaluator and
batch encoder to do encoding and NTT ops.
*/
BatchedPlaintextPolyn::BatchedPlaintextPolyn(
    const vector<FEltPolyn>& polyns, CryptoContext context, uint32_t ps_low_degree, bool compressed)
  : crypto_context(std::move(context))
{
    // compr_mode_type compr_mode = compressed ? compr_mode_type::zstd :
    // compr_mode_type::none;
    compr_mode_type compr_mode = compr_mode_type::none;
    // Find the highest degree polynomial in the list. The max degree determines how many
    // Plaintexts we need to make
    size_t max_deg = 0;
    for (const FEltPolyn& p : polyns)
    {
        // Degree = number of coefficients - 1
        max_deg = max(p.size(), max_deg + 1) - 1;
    }

    // We will encode with parameters that leave one or two levels, depending on whether
    // Paterson-Stockmeyer is used.
    size_t plain_coeffs_chain_idx = min<size_t>(
        crypto_context.seal_context()->first_context_data()->chain_index(), ps_low_degree ? 2 : 1);
    auto encode_parms_id =
        get_parms_id_for_chain_idx(*crypto_context.seal_context(), plain_coeffs_chain_idx);

    // Now make the Plaintexts. We let Plaintext i contain all bin coefficients of degree i.
    size_t num_polyns = polyns.size();
    for (size_t i = 0; i < max_deg + 1; i++)
    {
        // Go through all the bins, collecting the coefficients at degree i
        vector<felt_t> coeffs_of_deg_i;
        coeffs_of_deg_i.reserve(num_polyns);
        for (const FEltPolyn& p : polyns)
        {
            // Get the coefficient if it's set. Otherwise it's zero
            felt_t coeff = 0;
            if (i < p.size())
            {
                coeff = p[i];
            }

            coeffs_of_deg_i.push_back(coeff);
        }

        // Now let pt be the Plaintext consisting of all those degree i coefficients
        Plaintext pt;
        crypto_context.encoder()->encode(coeffs_of_deg_i, pt);

        // When evaluating the match and interpolation polynomials on encrypted query data,
        // we multiply each power of the encrypted query with a plaintext (pt here)
        // corresponding to the polynomial coefficient, and add the results together. The
        // constant coefficient is handled by simply adding to the result,  which requires
        // that the plaintext is not in NTT form. When Paterson-Stockmeyer is used, this
        // applies also to the constant coefficients for all inner polynomials, i.e., with
        // i a multiple of the ps_high_degree == ps_low_degree + 1.
        if ((!ps_low_degree && (i != 0)) || (ps_low_degree && (i % (ps_low_degree + 1))))
        {
            crypto_context.evaluator()->transform_to_ntt_inplace(pt, encode_parms_id);
        }

        // Push the new Plaintext
        vector<unsigned char> pt_data;
        pt_data.resize(safe_cast<size_t>(pt.save_size(compr_mode)));
        size_t size = static_cast<size_t>(
            pt.save(reinterpret_cast<seal_byte*>(pt_data.data()), pt_data.size(), compr_mode));
        pt_data.resize(size);
        batched_coeffs.push_back(std::move(pt_data));
    }
}

BinBundleCache::BinBundleCache(const CryptoContext& crypto_context, size_t label_size)
  : batched_matching_polyn(crypto_context)
{
    batched_interp_polyns.reserve(label_size);
    for (size_t label_idx = 0; label_idx < label_size; label_idx++)
    {
        batched_interp_polyns.emplace_back(crypto_context);
    }
}

BinBundle::BinBundle(const CryptoContext& crypto_context, size_t label_size, size_t max_bin_size,
    size_t ps_low_degree, size_t num_bins, bool compressed, bool stripped)
  : cache_invalid_(true),
    crypto_context_(crypto_context),
    compressed_(compressed),
    label_size_(label_size),
    max_bin_size_(max_bin_size),
    ps_low_degree_(ps_low_degree),
    num_bins_(num_bins),
    cache_(crypto_context_, label_size_)
{
    if (!crypto_context_.evaluator())
    {
        throw invalid_argument("evaluator is not set in crypto_context");
    }
    if (ps_low_degree > max_bin_size)
    {
        throw invalid_argument("ps_low_degree cannot be larger than max_bin_size");
    }
    if (!num_bins)
    {
        throw invalid_argument("num_bins cannot be zero");
    }

    // Set up internal data structures
    clear(stripped);
}

/**
Returns the modulus that defines the finite field that we're working in
*/
const Modulus& BinBundle::field_mod() const
{
    const auto& context_data = crypto_context_.seal_context()->first_context_data();
    return context_data->parms().plain_modulus();
}

template <>
int32_t BinBundle::multi_insert(const vector<felt_t>& items, size_t start_bin_idx, bool dry_run)
{
    if (stripped_)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("Cannot insert data to a stripped BinBundle");
        throw logic_error("failed to insert data");
    }
    if (items.empty())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("No item data to insert");
        return -1;
    }

    // We are inserting items only; no labels. This BinBundle cannot have a non-zero label
    // size.
    if (get_label_size())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC(
            "Attempted to insert unlabeled data in a labeled BinBundle");
        throw logic_error("failed to insert data");
    }

    // Return -1 if there isn't enough room in the BinBundle to insert at the given location
    if (start_bin_idx >= get_num_bins() || items.size() > get_num_bins() - start_bin_idx)
    {
        return -1;
    }

    // If we're here, that means we can insert in all bins
    size_t max_bin_size = 0;
    size_t curr_bin_idx = start_bin_idx;
    for (felt_t curr_item : items)
    {
        vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];

        // Compare the would-be bin size here to the running max
        if (max_bin_size < curr_bin.size() + 1)
        {
            max_bin_size = curr_bin.size() + 1;
        }

        // Insert if not dry run
        if (!dry_run)
        {
            // Insert the new item
            CuckooFilter& curr_filter = filters_[curr_bin_idx];
            curr_bin.push_back(curr_item);
            curr_filter.add(curr_item);

            // Indicate that the polynomials need to be recomputed
            cache_invalid_ = true;
        }

        curr_bin_idx++;
    }

    return safe_cast<int32_t>(max_bin_size);
}

template <>
int32_t BinBundle::multi_insert(
    const vector<pair<felt_t, vector<felt_t>>>& item_labels, size_t start_bin_idx, bool dry_run)
{
    if (stripped_)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("Cannot insert data to a stripped BinBundle");
        throw logic_error("failed to insert data");
    }
    if (item_labels.empty())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("No item or label data to insert");
        return -1;
    }

    // We are inserting item-labels. This BinBundle cannot have a zero label size.
    if (!get_label_size())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC(
            "Attempted to insert labeled data in an unlabeled BinBundle");
        throw logic_error("failed to insert data");
    }

    // Check that item_labels has correct size
    size_t label_size = get_label_size();
    for (const auto& curr_item_label : item_labels)
    {
        size_t curr_label_size = curr_item_label.second.size();
        if (curr_label_size != label_size)
        {
            LABELED_PSI_LOG(ERROR)
                << LOG_DESC("Attempted to insert item-label with incorrect label size ")
                << LOG_KV("curr_label_size", curr_label_size) << LOG_KV("expected", label_size);
            throw invalid_argument("failed to insert data");
        }
    }

    // Return -1 if there isn't enough room in the BinBundle to insert at the given location
    if (start_bin_idx >= get_num_bins() || item_labels.size() > get_num_bins() - start_bin_idx)
    {
        return -1;
    }

    // Do we have a non-zero label size? In that case we cannot have repeated item parts in
    // bins
    if (get_label_size())
    {
        // For each key, check that we can insert into the corresponding bin. If the answer
        // is "no" at any point, return -1.
        size_t curr_bin_idx = start_bin_idx;
        for (auto& curr_item_label : item_labels)
        {
            felt_t curr_item = curr_item_label.first;
            vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];
            CuckooFilter& curr_filter = filters_[curr_bin_idx];

            // Check if the key is already in the current bin. If so, that's an insertion
            // error
            if (is_present(curr_bin, curr_filter, curr_item))
            {
                return -1;
            }

            curr_bin_idx++;
        }
    }

    // If we're here, that means we can insert in all bins
    size_t max_bin_size = 0;
    size_t curr_bin_idx = start_bin_idx;
    for (auto& curr_item_label : item_labels)
    {
        felt_t curr_item = curr_item_label.first;
        vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];

        // Compare the would-be bin size here to the running max
        if (max_bin_size < curr_bin.size() + 1)
        {
            max_bin_size = curr_bin.size() + 1;
        }

        // Insert if not dry run
        if (!dry_run)
        {
            // Insert the new item
            CuckooFilter& curr_filter = filters_[curr_bin_idx];
            curr_bin.push_back(curr_item);
            curr_filter.add(curr_item);

            // Insert the new label; loop over each label part
            for (size_t label_idx = 0; label_idx < get_label_size(); label_idx++)
            {
                // Add this label part to the matching bin
                felt_t curr_label = curr_item_label.second[label_idx];
                label_bins_[label_idx][curr_bin_idx].push_back(curr_label);
            }

            // Indicate that the polynomials need to be recomputed
            cache_invalid_ = true;
        }

        curr_bin_idx++;
    }

    return safe_cast<int>(max_bin_size);
}

template <>
bool BinBundle::try_multi_overwrite(const vector<felt_t>& items, size_t start_bin_idx)
{
    if (stripped_)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("Cannot overwrite data in a stripped BinBundle");
        throw logic_error("failed to overwrite data");
    }
    if (items.empty())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("No item data to insert");
        return false;
    }

    // This function may have been called accidentally; no label data is given, so nothing
    // will be overwritten. This is equivalent to searching for the presence of the felt
    // items in this BinBundle and forcing the cache to be recomputed.
    // Return false if there isn't enough room in the BinBundle to insert at the given
    // location
    if (start_bin_idx >= get_num_bins() || items.size() > get_num_bins() - start_bin_idx)
    {
        return false;
    }

    // Check that all the item components appear sequentially in this BinBundle
    size_t curr_bin_idx = start_bin_idx;
    for (felt_t curr_item : items)
    {
        vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];
        CuckooFilter& curr_filter = filters_[curr_bin_idx];

        // A non-match was found; the item is not here.
        if (!is_present(curr_bin, curr_filter, curr_item))
        {
            return false;
        }

        curr_bin_idx++;
    }

    // Nothing was done, but mark the cache as dirty anyway
    cache_invalid_ = true;

    return true;
}

template <>
bool BinBundle::try_multi_overwrite(
    const vector<pair<felt_t, vector<felt_t>>>& item_labels, size_t start_bin_idx)
{
    if (stripped_)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("Cannot overwrite data in a stripped BinBundle");
        throw logic_error("failed to overwrite data");
    }
    if (item_labels.empty())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("No item or label data to insert");
        return false;
    }

    // Check that item_labels has correct size
    size_t label_size = get_label_size();
    for (const auto& curr_item_label : item_labels)
    {
        size_t curr_label_size = curr_item_label.second.size();
        if (curr_label_size != label_size)
        {
            LABELED_PSI_LOG(ERROR)
                << LOG_DESC("Attempted to overwrite item-label with incorrect label size ")
                << LOG_KV("curr_label_size", curr_label_size) << LOG_KV("expected", label_size);

            throw invalid_argument("failed to overwrite data");
        }
    }

    // Return false if there isn't enough room in the BinBundle to insert at the given
    // location
    if (start_bin_idx >= get_num_bins() || item_labels.size() > get_num_bins() - start_bin_idx)
    {
        return false;
    }

    // Check that all the item components appear sequentially in this BinBundle
    size_t curr_bin_idx = start_bin_idx;
    for (auto& curr_item_label : item_labels)
    {
        felt_t curr_item = curr_item_label.first;
        vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];
        CuckooFilter& curr_filter = filters_[curr_bin_idx];

        // A non-match was found; the item is not here.
        if (!is_present(curr_bin, curr_filter, curr_item))
        {
            return false;
        }

        curr_bin_idx++;
    }

    // If we're here, that means we can overwrite the labels
    curr_bin_idx = start_bin_idx;
    for (auto& curr_item_label : item_labels)
    {
        felt_t curr_item = curr_item_label.first;

        // Overwrite the label in the bin
        vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];

        // No point in using cuckoo filters here for look-up: we know the item exists so do
        // linear search
        auto found_pos = find(curr_bin.begin(), curr_bin.end(), curr_item);

        // From the earlier check we know that found_pos is not the end-iterator. Check this
        // again to be sure.
        if (found_pos == curr_bin.end())
        {
            LABELED_PSI_LOG(ERROR) << LOG_DESC(
                "Attempted to overwrite item-label, but the item could no longer be found; "
                "the internal state of this BinBundle has been corrupted");
            throw runtime_error("failed to overwrite data");
        }

        // Compute the location in the curr_bin so we know how to index into the label bins
        auto item_idx_in_bin = static_cast<size_t>(distance(curr_bin.begin(), found_pos));

        // Write the new label; loop over each label part
        for (size_t label_idx = 0; label_idx < get_label_size(); label_idx++)
        {
            // Overwrite this label part in the matching bin
            felt_t curr_label = curr_item_label.second[label_idx];
            label_bins_[label_idx][curr_bin_idx][item_idx_in_bin] = curr_label;
        }

        // Indicate that the polynomials need to be recomputed
        cache_invalid_ = true;

        curr_bin_idx++;
    }

    return true;
}

bool BinBundle::try_multi_remove(const vector<felt_t>& items, size_t start_bin_idx)
{
    if (stripped_)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("Cannot remove data from a stripped BinBundle");
        throw logic_error("failed to remove data");
    }
    if (items.empty())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("No item data to remove");
        return false;
    }

    // Return false if there isn't enough room in the BinBundle at the given location
    if (start_bin_idx >= get_num_bins() || items.size() > get_num_bins() - start_bin_idx)
    {
        return false;
    }

    // Go through all the items. If any item doesn't appear, we scrap the whole computation
    // and return false.
    size_t curr_bin_idx = start_bin_idx;
    vector<vector<felt_t>::iterator> to_remove_item_its;
    vector<vector<vector<felt_t>::iterator>> to_remove_label_its(get_label_size());

    for (auto& item : items)
    {
        vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];
        CuckooFilter& curr_filter = filters_[curr_bin_idx];

        auto to_remove_item_it = get_iterator(curr_bin, curr_filter, item);
        if (curr_bin.end() == to_remove_item_it)
        {
            // One of the items isn't there; return false;
            return false;
        }
        else
        {
            // Found the item; mark it for removal
            to_remove_item_its.push_back(to_remove_item_it);

            // We need to also mark the corresponding labels for removal
            auto item_loc_in_bin = distance(curr_bin.begin(), to_remove_item_it);
            for (size_t label_idx = 0; label_idx < get_label_size(); label_idx++)
            {
                auto to_remove_label_it =
                    label_bins_[label_idx][curr_bin_idx].begin() + item_loc_in_bin;
                to_remove_label_its[label_idx].push_back(to_remove_label_it);
            }
        }

        curr_bin_idx++;
    }

    // We got to this point, so all of the items were found. Now just erase them.
    curr_bin_idx = start_bin_idx;
    for (auto to_remove_item_it : to_remove_item_its)
    {
        // Remove the item
        filters_[curr_bin_idx].remove(*to_remove_item_it);
        item_bins_[curr_bin_idx].erase(to_remove_item_it);

        // Indicate that the polynomials need to be recomputed
        cache_invalid_ = true;

        curr_bin_idx++;
    }

    // Finally erase the label parts
    for (size_t label_idx = 0; label_idx < get_label_size(); label_idx++)
    {
        curr_bin_idx = start_bin_idx;
        for (auto to_remove_label_it : to_remove_label_its[label_idx])
        {
            // Remove the label
            label_bins_[label_idx][curr_bin_idx].erase(to_remove_label_it);

            curr_bin_idx++;
        }
    }

    return true;
}

bool BinBundle::try_get_multi_label(
    const vector<felt_t>& items, size_t start_bin_idx, vector<felt_t>& labels) const
{
    if (stripped_)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("Cannot retrieve labels from a stripped BinBundle");
        throw logic_error("failed to retrieve labels");
    }
    if (items.empty())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("No item data to search for");
        return false;
    }

    // Return false if there isn't enough room in the BinBundle at the given location
    if (start_bin_idx >= get_num_bins() || items.size() > get_num_bins() - start_bin_idx)
    {
        return false;
    }

    // Resize the labels vector to expected size; we will write in a non-linear order
    labels.clear();
    labels.resize(items.size() * get_label_size());

    // Go through all the items. If the item appears, find its label and write to labels. If
    // any item doesn't appear, we scrap the whole computation and return false.
    size_t curr_bin_idx = start_bin_idx;
    for (size_t item_idx = 0; item_idx < items.size(); item_idx++)
    {
        const vector<felt_t>& curr_bin = item_bins_[curr_bin_idx];
        const CuckooFilter& curr_filter = filters_[curr_bin_idx];

        // Find the item if present in this bin
        auto item_it = get_iterator(curr_bin, curr_filter, items[item_idx]);

        if (curr_bin.end() == item_it)
        {
            // One of the items isn't there. No label to fetch. Clear the labels and return
            // early.
            labels.clear();
            return false;
        }

        // Found the (felt) item. Next collect the label parts for this and write to label.
        size_t item_idx_in_bin = static_cast<size_t>(distance(curr_bin.begin(), item_it));
        for (size_t label_idx = 0; label_idx < get_label_size(); label_idx++)
        {
            // Need to reorder the felts
            labels[items.size() * label_idx + item_idx] =
                label_bins_[label_idx][curr_bin_idx][item_idx_in_bin];
        }

        curr_bin_idx++;
    }

    return true;
}

void BinBundle::clear(bool stripped)
{
    // Set the stripped flag
    stripped_ = stripped;

    // Clear item data
    item_bins_.clear();
    if (!stripped_)
    {
        item_bins_.resize(num_bins_);
    }

    // Clear label data
    label_bins_.clear();
    if (!stripped_)
    {
        label_bins_.reserve(label_size_);
        for (size_t i = 0; i < label_size_; i++)
        {
            label_bins_.emplace_back(num_bins_);
        }
    }

    // Clear filters
    filters_.clear();
    if (!stripped_)
    {
        filters_.reserve(num_bins_);
        for (size_t i = 0; i < num_bins_; i++)
        {
            filters_.emplace_back(max_bin_size_, /* bits per tag */ 12);
        }
    }

    // Clear the cache
    clear_cache();
}

void BinBundle::clear_cache()
{
    cache_.felt_matching_polyns.clear();
    cache_.batched_matching_polyn = crypto_context_;

    cache_.felt_interp_polyns.clear();
    cache_.batched_interp_polyns.clear();

    cache_invalid_ = true;
}

const BinBundleCache& BinBundle::get_cache() const
{
    if (cache_invalid_)
    {
        throw logic_error("tried to retrieve stale cache");
    }

    return cache_;
}

void BinBundle::regen_plaintexts()
{
    // This function assumes that BinBundle::clear_cache and BinBundle::regen_polyns have
    // been called and the polynomials have not been modified since then.

    // Allocate memory for the batched "label polynomials"
    cache_.batched_interp_polyns.resize(get_label_size());

    ThreadPoolMgr tpm;

    vector<future<void>> futures;
    futures.push_back(tpm.thread_pool().enqueue([&]() {
        // Compute and cache the batched "matching polynomials". They're computed in both
        // labeled and unlabeled PSI.
        BatchedPlaintextPolyn bmp(cache_.felt_matching_polyns, crypto_context_,
            static_cast<uint32_t>(ps_low_degree_), compressed_);
        cache_.batched_matching_polyn = std::move(bmp);
    }));

    for (size_t label_idx = 0; label_idx < cache_.felt_interp_polyns.size(); label_idx++)
    {
        futures.push_back(tpm.thread_pool().enqueue([&, label_idx]() {
            // Compute and cache the batched Newton interpolation polynomials
            const auto& interp_polyn = cache_.felt_interp_polyns[label_idx];
            BatchedPlaintextPolyn bip(
                interp_polyn, crypto_context_, static_cast<uint32_t>(ps_low_degree_), compressed_);
            cache_.batched_interp_polyns[label_idx] = std::move(bip);
        }));
    }

    // Wait for the tasks to finish
    for (auto& f : futures)
    {
        f.get();
    }
}

void BinBundle::regen_polyns()
{
    // This function assumes that BinBundle::clear_cache has been called and the polynomials
    // have not been modified since then. Specifically, it assumes that item_bins_ is empty
    // and and label_bins_ is set to the correct size.

    // Get the field modulus. We need this for polynomial calculations
    const Modulus& mod = field_mod();

    size_t num_bins = get_num_bins();
    size_t label_size = get_label_size();
    cache_.felt_matching_polyns.resize(num_bins);
    cache_.felt_interp_polyns.resize(label_size);
    for (auto& fips : cache_.felt_interp_polyns)
    {
        fips.resize(num_bins);
    }

    ThreadPoolMgr tpm;

    vector<future<void>> futures;
    // For each bin in the bundle, compute and cache the corresponding "matching
    // polynomial"
    for (size_t bin_idx = 0; bin_idx < num_bins; bin_idx++)
    {
        futures.push_back(tpm.thread_pool().enqueue([&, bin_idx]() {
            // Compute and cache the matching polynomial
            FEltPolyn fmp = polyn_with_roots(item_bins_[bin_idx], mod);
            cache_.felt_matching_polyns[bin_idx] = std::move(fmp);
        }));
    }

    // For each bin in the bundle, compute and cache the corresponding "label polynomials"
    for (size_t label_idx = 0; label_idx < label_size; label_idx++)
    {
        for (size_t bin_idx = 0; bin_idx < num_bins; bin_idx++)
        {
            futures.push_back(tpm.thread_pool().enqueue([&, label_idx, bin_idx]() {
                // Compute and cache the matching polynomial
                FEltPolyn fip = newton_interpolate_polyn(
                    item_bins_[bin_idx], label_bins_[label_idx][bin_idx], mod);
                cache_.felt_interp_polyns[label_idx][bin_idx] = std::move(fip);
            }));
        }
    }

    // Wait for the tasks to finish
    for (auto& f : futures)
    {
        f.get();
    }
}

void BinBundle::regen_cache()
{
    // Only recompute the cache if it needs to be recomputed
    if (cache_invalid_)
    {
        clear_cache();
        regen_polyns();
        regen_plaintexts();
        cache_invalid_ = false;
    }
}

bool BinBundle::empty() const
{
    return all_of(item_bins_.begin(), item_bins_.end(), [](auto& b) { return b.empty(); });
}

void BinBundle::strip()
{
    // Ensure the cache is valid
    regen_cache();

    stripped_ = true;

    item_bins_.clear();
    label_bins_.clear();
    filters_.clear();

    cache_.felt_matching_polyns.clear();
    cache_.felt_interp_polyns.clear();
}

ppctars::BinBundle BinBundle::saveToTarsBinBundle() const
{
    ppctars::BinBundle binBundle;

    // write the items and labels
    for (auto& bins : item_bins_)
    {
        std::vector<tars::Int64> tarsBins;
        tarsBins.reserve(bins.size());
        for (auto& item : bins)
        {
            tarsBins.emplace_back(item);
        }
        binBundle.itemBins.emplace_back(tarsBins);
    }
    for (auto& bins : label_bins_)
    {
        std::vector<std::vector<tars::Int64>> tarsBins;
        for (auto& labels : bins)
        {
            std::vector<tars::Int64> tarsLabels;
            tarsLabels.reserve(labels.size());
            for (auto& label : labels)
            {
                tarsLabels.emplace_back(label);
            }
            tarsBins.emplace_back(tarsLabels);
        }
        binBundle.labelBins.emplace_back(tarsBins);
    }

    // write BinBundleCache
    ppctars::BinBundleCache binBundleCache;

    for (auto& polyns : cache_.felt_matching_polyns)
    {
        std::vector<tars::Int64> tarsPolyns;
        tarsPolyns.reserve(polyns.size());
        for (auto& polyn : polyns)
        {
            tarsPolyns.emplace_back(polyn);
        }
        binBundleCache.feltMatchingPolyns.emplace_back(tarsPolyns);
    }
    for (auto& interpPolyns : cache_.felt_interp_polyns)
    {
        std::vector<std::vector<tars::Int64>> tarsInterpPolyns;
        for (auto& polyns : interpPolyns)
        {
            std::vector<tars::Int64> tarsPolyns;
            tarsPolyns.reserve(polyns.size());
            for (auto& polyn : polyns)
            {
                tarsPolyns.emplace_back(polyn);
            }
            tarsInterpPolyns.emplace_back(tarsPolyns);
        }
        binBundleCache.feltInterpPolyns.emplace_back(tarsInterpPolyns);
    }
    binBundleCache.batchedMatchingPolyn.batchedCoeffs =
        cache_.batched_matching_polyn.batched_coeffs;
    for (auto& polyns : cache_.batched_interp_polyns)
    {
        ppctars::BatchedPlaintextPolyn batchedPlaintextPolyn;
        batchedPlaintextPolyn.batchedCoeffs = polyns.batched_coeffs;
        binBundleCache.batchedInterpPolyns.emplace_back(batchedPlaintextPolyn);
    }
    binBundle.cache = binBundleCache;

    binBundle.cacheInvalid = cache_invalid_;
    binBundle.compressed = compressed_;
    binBundle.stripped = stripped_;
    binBundle.labelSize = label_size_;
    binBundle.maxBinSize = max_bin_size_;
    binBundle.psLowDegree = ps_low_degree_;
    binBundle.numBins = num_bins_;

    return binBundle;
}

BinBundle BinBundle::loadFromTarsBinBundle(
    const CryptoContext& _cryptoContext, const ppctars::BinBundle& _tarsBinBundle)
{
    BinBundle binBundle(_cryptoContext, _tarsBinBundle.labelSize, _tarsBinBundle.maxBinSize,
        _tarsBinBundle.psLowDegree, _tarsBinBundle.numBins, _tarsBinBundle.compressed,
        _tarsBinBundle.stripped);
    binBundle.cache_invalid_ = _tarsBinBundle.cacheInvalid;

    // load the items and labels
    binBundle.item_bins_.clear();
    for (auto& tarsBins : _tarsBinBundle.itemBins)
    {
        std::vector<uint64_t> bins;
        bins.reserve(tarsBins.size());
        for (auto& item : tarsBins)
        {
            bins.emplace_back(item);
        }
        binBundle.item_bins_.emplace_back(bins);
    }

    binBundle.label_bins_.clear();
    for (auto& tarsBins : _tarsBinBundle.labelBins)
    {
        std::vector<std::vector<uint64_t>> bins;
        for (auto& tarsLabels : tarsBins)
        {
            std::vector<uint64_t> labels;
            labels.reserve(tarsLabels.size());
            for (auto& label : tarsLabels)
            {
                labels.emplace_back(label);
            }
            bins.emplace_back(labels);
        }
        binBundle.label_bins_.emplace_back(bins);
    }

    // init CuckooFilter
    auto binSize = binBundle.item_bins_.size();
    for (size_t binIdx = 0; !binBundle.stripped_ && (binIdx < binSize); binIdx++)
    {
        auto& items = binBundle.item_bins_[binIdx];
        for (auto& felt : items)
        {
            binBundle.filters_[binIdx].add(felt);
        }
    }

    // init BinBundleCache
    for (auto& tarsPolyns : _tarsBinBundle.cache.feltMatchingPolyns)
    {
        std::vector<uint64_t> polyns;
        polyns.reserve(tarsPolyns.size());
        for (auto& polyn : tarsPolyns)
        {
            polyns.emplace_back(polyn);
        }
        binBundle.cache_.felt_matching_polyns.emplace_back(polyns);
    }
    for (auto& tarsInterpPolyns : _tarsBinBundle.cache.feltInterpPolyns)
    {
        std::vector<std::vector<uint64_t>> interpPolyns;
        for (auto& tarsPolyns : tarsInterpPolyns)
        {
            std::vector<uint64_t> polyns;
            polyns.reserve(tarsPolyns.size());
            for (auto& polyn : tarsPolyns)
            {
                polyns.emplace_back(polyn);
            }
            interpPolyns.emplace_back(polyns);
        }
        binBundle.cache_.felt_interp_polyns.emplace_back(interpPolyns);
    }
    binBundle.cache_.batched_matching_polyn.batched_coeffs =
        _tarsBinBundle.cache.batchedMatchingPolyn.batchedCoeffs;
    for (auto& polyn : _tarsBinBundle.cache.batchedInterpPolyns)
    {
        BatchedPlaintextPolyn batchedPlaintextPolyn;
        batchedPlaintextPolyn.batched_coeffs = polyn.batchedCoeffs;
        batchedPlaintextPolyn.crypto_context = _cryptoContext;
        binBundle.cache_.batched_interp_polyns.emplace_back(std::move(batchedPlaintextPolyn));
    }

    return binBundle;
}
