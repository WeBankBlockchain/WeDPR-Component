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
 * @reference https://github.com/secretflow/spu
 * @license Apache license
 *
 * @file LabeledPSIParams.h
 * @author: shawnhe
 * @date 2022-11-14
 *
 */

#pragma once

#include "LabeledPSI.h"
#include "ppc-psi/src/labeled-psi/Common.h"

#include <apsi/psi_params.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace ppc::psi
{
struct BaseParams
{
    size_t maxItemsPerBin;
    size_t polyModulusDegree;
    // plain_modulus or plain_modulus_bits
    size_t plainModulus = 0;
    size_t plainModulusBits = 0;
    std::vector<int> coeffModulusBits;


    size_t getPlainModulusBits()
    {
        // plain_modulus_bits
        if (plainModulusBits > 0)
        {
            return plainModulusBits;
        }
        else if (plainModulus > 0)
        {
            // get plain_modulus_bits by plain_modulus
            return std::floor(std::log2(plainModulus));
        }
        return 0;
    }
};

inline size_t getHashTruncateSize(size_t _senderSize, size_t _statsSecParams = 40)
{
    // reference: Fast Private Set Intersection from Homomorphic Encryption, section 4.2
    // url: https://eprint.iacr.org/2017/299
    size_t tSize =
        std::ceil(std::log2(_senderSize)) + std::ceil(std::log2(MAX_QUERY_SIZE)) + _statsSecParams;
    return std::max(static_cast<size_t>(80), tSize);
}

inline apsi::PSIParams getPsiParams(size_t _senderSize)
{
    // reference: microsoft APSI example parameter sets
    // url: https://github.com/microsoft/APSI/tree/main/parameters
    std::vector<BaseParams> kBaseParams = {
        {42, 2048, 65537, 0, {48}},             // 0 100k-1-16
        {228, 8192, 65537, 0, {56, 48, 48}},    // 1 1M-1-32
        {782, 8192, 0, 21, {56, 56, 56, 50}},   // 2 16M-1-32
        {8100, 8192, 0, 22, {56, 56, 56, 32}},  // 3 256M-1
    };

    BaseParams baseParams;
    if (_senderSize <= (1 << 18))
    {
        baseParams = kBaseParams[0];
    }
    else if (_senderSize <= (1 << 20))
    {
        baseParams = kBaseParams[1];
    }
    else if (_senderSize <= (1 << 24))
    {
        baseParams = kBaseParams[2];
    }
    else
    {
        baseParams = kBaseParams[3];
    }

    // item param
    apsi::PSIParams::ItemParams itemParams{};
    size_t hash_size = getHashTruncateSize(_senderSize);
    itemParams.felts_per_item =
        std::ceil(static_cast<double>(hash_size) / (baseParams.getPlainModulusBits() - 1));

    // table param
    apsi::PSIParams::TableParams tableParams{};
    tableParams.max_items_per_bin = baseParams.maxItemsPerBin;
    tableParams.hash_func_count = 1;

    size_t polyItemCount = baseParams.polyModulusDegree / itemParams.felts_per_item;
    tableParams.table_size = polyItemCount;

    size_t cuckooTableSize = std::ceil(4 * MAX_QUERY_SIZE);
    while (tableParams.table_size < cuckooTableSize)
    {
        tableParams.table_size += polyItemCount;
    }

    // query param
    apsi::PSIParams::QueryParams queryParams;
    if (tableParams.max_items_per_bin == 42)
    {
        queryParams.ps_low_degree = 0;
        queryParams.query_powers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42};
    }
    else if (tableParams.max_items_per_bin == 228)
    {
        queryParams.ps_low_degree = 0;
        queryParams.query_powers = {1, 3, 8, 19, 33, 39, 92, 102};
    }
    else if (tableParams.max_items_per_bin == 782)
    {
        queryParams.ps_low_degree = 26;
        queryParams.query_powers = {1, 5, 8, 27, 135};
    }
    else
    {
        queryParams.ps_low_degree = 310;
        queryParams.query_powers = {1, 4, 10, 11, 28, 33, 78, 118, 143, 311, 1555};
    }

    // seal param
    apsi::PSIParams::SEALParams apsiSealParams;
    apsiSealParams.set_poly_modulus_degree(baseParams.polyModulusDegree);

    if (baseParams.plainModulusBits > 0)
    {
        apsiSealParams.set_plain_modulus(seal::PlainModulus::Batching(
            baseParams.polyModulusDegree, baseParams.plainModulusBits));
    }
    else
    {
        apsiSealParams.set_plain_modulus(baseParams.plainModulus);
    }

    apsiSealParams.set_coeff_modulus(
        seal::CoeffModulus::Create(baseParams.polyModulusDegree, baseParams.coeffModulusBits));

    apsi::PSIParams psi_params(itemParams, tableParams, queryParams, apsiSealParams);

    return psi_params;
}

}  // namespace ppc::psi