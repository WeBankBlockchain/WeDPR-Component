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
 * @file Protocol.h
 * @author: shawnhe
 * @date 2022-11-6
 */

#pragma once
#include "LabeledPSI.h"
#include "ppc-framework/Common.h"

#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>

#include <apsi/psi_params.h>

namespace ppc::psi
{
inline apsi::PSIParams toPSIParams(const ppctars::PsiParams& _tarsParams)
{
    apsi::PSIParams::ItemParams itemParams{};
    apsi::PSIParams::TableParams tableParams{};
    apsi::PSIParams::QueryParams queryParams;
    apsi::PSIParams::SEALParams sealParams;

    itemParams.felts_per_item = _tarsParams.feltsPerItem;

    tableParams.hash_func_count = _tarsParams.hashFuncCount;
    tableParams.table_size = _tarsParams.tableSize;
    tableParams.max_items_per_bin = _tarsParams.maxItemsPerBin;

    queryParams.ps_low_degree = _tarsParams.psLowDegree;

    if (!_tarsParams.queryPowers.empty())
    {
        for (auto queryPower : _tarsParams.queryPowers)
        {
            queryParams.query_powers.insert(queryPower);
        }
    }
    else
    {
        for (uint idx = 1; idx <= tableParams.max_items_per_bin; ++idx)
        {
            queryParams.query_powers.insert(idx);
        }
    }

    std::vector<seal::Modulus> coeffModulus;
    coeffModulus.insert(coeffModulus.end(), _tarsParams.sealParams.coeffModulus.begin(),
        _tarsParams.sealParams.coeffModulus.end());

    sealParams.set_coeff_modulus(coeffModulus);
    sealParams.set_poly_modulus_degree(_tarsParams.sealParams.polyModulusDegree);
    sealParams.set_plain_modulus(_tarsParams.sealParams.plainModulus);

    apsi::PSIParams psiParams(itemParams, tableParams, queryParams, sealParams);
    return psiParams;
}

inline ppctars::PsiParams fromPSIParams(const apsi::PSIParams& _psiParams, uint32_t _binBundleCount)
{
    ppctars::PsiParams tarsParams;
    tarsParams.hashFuncCount = _psiParams.table_params().hash_func_count;
    tarsParams.tableSize = _psiParams.table_params().table_size;
    tarsParams.maxItemsPerBin = _psiParams.table_params().max_items_per_bin;

    tarsParams.feltsPerItem = _psiParams.item_params().felts_per_item;

    tarsParams.psLowDegree = _psiParams.query_params().ps_low_degree;
    for (auto& power : _psiParams.query_params().query_powers)
    {
        tarsParams.queryPowers.emplace_back(power);
    }

    ppctars::SealParams tarsSealParams;
    tarsSealParams.plainModulus = _psiParams.seal_params().plain_modulus().value();
    tarsSealParams.polyModulusDegree = _psiParams.seal_params().poly_modulus_degree();
    for (auto& modulu : _psiParams.seal_params().coeff_modulus())
    {
        tarsSealParams.coeffModulus.emplace_back(modulu.value());
    }
    tarsParams.sealParams = tarsSealParams;

    tarsParams.binBundleCount = _binBundleCount;
    return tarsParams;
}

}  // namespace ppc::psi
