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
 * @file EcdhOprf.cpp
 * @author: shawnhe
 * @date 2022-11-3
 */

#include "EcdhOprf.h"
#include "../Common.h"
#include <tbb/parallel_for.h>

using namespace ppc::protocol;
using namespace ppc::crypto;

// H1(_input)^r
bcos::bytes ppc::crypto::EcdhOprfClient::blind(std::string_view _input) const
{
    bcos::bytesConstRef inputData((const unsigned char*)_input.data(), _input.size());
    auto hashResult = m_hash->hash(inputData);
    auto ecPoint = m_eccCrypto->hashToCurve(hashResult);

    return m_eccCrypto->ecMultiply(ecPoint, m_privateKey);
}

bcos::bytes ppc::crypto::EcdhOprfClient::blind(const bcos::bytes& _input) const
{
    bcos::bytesConstRef inputData(_input.data(), _input.size());
    auto hashResult = m_hash->hash(inputData);
    auto ecPoint = m_eccCrypto->hashToCurve(hashResult);

    return m_eccCrypto->ecMultiply(ecPoint, m_privateKey);
}

std::vector<bcos::bytes> ppc::crypto::EcdhOprfClient::blind(
    const std::vector<std::string>& _inputs) const
{
    auto inputsSize = _inputs.size();

    std::vector<bcos::bytes> results;
    results.reserve(inputsSize);
    results.resize(inputsSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputsSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            results[i] = blind(_inputs[i]);
        }
    });
    return results;
}

// H2(_input,_evaluatedItem^(1/r))
bcos::bytes ppc::crypto::EcdhOprfClient::finalize(
    std::string_view _input, const bcos::bytes& _evaluatedItem) const
{
    auto invertR = m_eccCrypto->scalarInvert(m_privateKey);
    auto unblindedItem = m_eccCrypto->ecMultiply(_evaluatedItem, invertR);

    auto hashState = m_hash->init();
    bcos::bytesConstRef inputData((const unsigned char*)_input.data(), _input.size());
    m_hash->update(hashState, inputData);
    m_hash->update(hashState,
        bcos::bytesConstRef((const unsigned char*)unblindedItem.data(), unblindedItem.size()));

    auto finalH = m_hash->final(hashState);
    finalH.resize(m_outputSize);
    return finalH;
}

std::vector<bcos::bytes> ppc::crypto::EcdhOprfClient::finalize(
    const std::vector<std::string>& _inputs, const std::vector<bcos::bytes>& _evaluatedItems) const
{
    auto inputsSize = _inputs.size();
    if (inputsSize != _evaluatedItems.size())
    {
        BOOST_THROW_EXCEPTION(OprfFinalizeException() << bcos::errinfo_comment(
                                  "The number of input does not match the evaluated element"));
    }

    std::vector<bcos::bytes> results;
    results.reserve(inputsSize);
    results.resize(inputsSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputsSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            results[i] = finalize(_inputs[i], _evaluatedItems[i]);
        }
    });

    return results;
}

// (_blindedItem)^sk
bcos::bytes EcdhOprfServer::evaluate(const bcos::bytes& _blindedItem) const
{
    return m_eccCrypto->ecMultiply(_blindedItem, m_privateKey);
}

std::vector<bcos::bytes> ppc::crypto::EcdhOprfServer::evaluate(
    const std::vector<bcos::bytes>& _blindedItems) const
{
    auto inputsSize = _blindedItems.size();

    std::vector<bcos::bytes> results;
    results.reserve(inputsSize);
    results.resize(inputsSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputsSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            results[i] = evaluate(_blindedItems[i]);
        }
    });

    return results;
}

// OPRF(_input, sk) = H2(x, H1(_input)^sk)
bcos::bytes ppc::crypto::EcdhOprfServer::fullEvaluate(const bcos::bytes& _input) const
{
    bcos::bytesConstRef inputData(_input.data(), _input.size());
    auto hashResult = m_hash->hash(inputData);
    auto ecPoint = m_eccCrypto->hashToCurve(hashResult);
    auto blindedItem = m_eccCrypto->ecMultiply(ecPoint, m_privateKey);

    auto hashState = m_hash->init();
    m_hash->update(hashState, inputData);
    m_hash->update(hashState,
        bcos::bytesConstRef((const unsigned char*)blindedItem.data(), blindedItem.size()));

    auto finalH = m_hash->final(hashState);
    finalH.resize(m_outputSize);
    return finalH;
}

bcos::bytes ppc::crypto::EcdhOprfServer::fullEvaluate(const std::string_view _input) const
{
    bcos::bytesConstRef inputData((const unsigned char*)_input.data(), _input.size());
    auto hashResult = m_hash->hash(inputData);
    auto ecPoint = m_eccCrypto->hashToCurve(hashResult);
    auto blindedItem = m_eccCrypto->ecMultiply(ecPoint, m_privateKey);

    auto hashState = m_hash->init();
    m_hash->update(hashState, inputData);
    m_hash->update(hashState,
        bcos::bytesConstRef((const unsigned char*)blindedItem.data(), blindedItem.size()));

    auto finalH = m_hash->final(hashState);
    finalH.resize(m_outputSize);
    return finalH;
}

std::vector<bcos::bytes> ppc::crypto::EcdhOprfServer::fullEvaluate(
    const std::vector<std::string>& _inputs) const
{
    auto inputsSize = _inputs.size();

    std::vector<bcos::bytes> results;
    results.reserve(inputsSize);
    results.resize(inputsSize);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputsSize), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            results[i] = fullEvaluate(_inputs[i]);
        }
    });
    return results;
}
