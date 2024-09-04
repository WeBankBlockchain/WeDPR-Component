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
 * @file RA2018Oprf.cpp
 * @author: yujiechen
 * @date 2022-12-6
 */
#include "RA2018Oprf.h"

using namespace ppc::crypto;
using namespace ppc::io;
using namespace bcos;

void RA2018Oprf::blind(ppc::io::DataBatch::Ptr const& _plainData, bcos::bytes const& _privateKey,
    std::vector<bcos::bytes>& _blindResult)
{
    // H1(yj)^Bj
    _blindResult.resize(_plainData->size());
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, _plainData->size()), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            blindImpl(_plainData->getBytes(i), _privateKey, _blindResult[i]);
        }
    });
}


void RA2018Oprf::finalize(std::vector<bcos::bytes> const& _evaluatedData,
    bcos::bytes const& _invPrivateKey, std::vector<bcos::bytes>& _finalizedResult)
{
    _finalizedResult.resize(_evaluatedData.size());
    // (aj)^(1/Bj)
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0U, _evaluatedData.size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                finalizeImpl(_invPrivateKey, _evaluatedData.at(i), _finalizedResult[i]);
            }
        });
}

void RA2018Oprf::fullEvaluate(
    ppc::io::DataBatch::Ptr const& _input, std::vector<bcos::bytes>& _result)
{
    _result.resize(_input->size());
    // H1(xi)^aj
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, _input->size()), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            fullEvaluateImpl(_input->get<bcos::bytes>(i), _result[i]);
        }
    });
}

void RA2018Oprf::evaluate(
    std::vector<bcos::bytes> const& _blindData, std::vector<bcos::bytes>& _result)
{
    _result.resize(_blindData.size());
    // (aj)^a
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, _blindData.size()), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            evaluateImpl(_blindData.at(i), _result[i]);
        }
    });
}
