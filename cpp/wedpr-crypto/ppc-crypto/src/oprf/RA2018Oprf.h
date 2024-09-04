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
 * @file RA2018Oprf.h
 * @author: yujiechen
 * @date 2022-12-6
 */
#pragma once
#include "../ecc/OpenSSLEccCrypto.h"
#include "ppc-framework/crypto/EccCrypto.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/crypto/RA2018OprfInterface.h"
#include "ppc-framework/io/DataBatch.h"
#include <bcos-utilities/Common.h>
#include <tbb/parallel_for.h>

namespace ppc::crypto
{
class RA2018Oprf : public RA2018OprfInterface
{
public:
    using Ptr = std::shared_ptr<RA2018Oprf>;
    RA2018Oprf(bcos::bytes _privateKey, EccCrypto::Ptr _eccCrypto, Hash::Ptr _hashImpl)
      : m_privateKey(std::move(_privateKey)), m_eccCrypto(_eccCrypto), m_hashImpl(_hashImpl)
    {}
    ~RA2018Oprf() override = default;

    bcos::bytes generatePrivateKey() override { return m_eccCrypto->generateRandomScalar(); }
    bcos::bytes inv(bcos::bytes const& _data) override { return m_eccCrypto->scalarInvert(_data); }

    void blind(ppc::io::DataBatch::Ptr const& _plainData, bcos::bytes const& _privateKey,
        std::vector<bcos::bytes>& _blindResult) override;

    void finalize(std::vector<bcos::bytes> const& _evaluatedData, bcos::bytes const& _invPrivateKey,
        std::vector<bcos::bytes>& _finalizedResult) override;

    void fullEvaluate(
        ppc::io::DataBatch::Ptr const& _input, std::vector<bcos::bytes>& _result) override;
    void evaluate(
        std::vector<bcos::bytes> const& _blindData, std::vector<bcos::bytes>& _result) override;


protected:
    virtual void blindImpl(
        bcos::bytes const& _input, bcos::bytes const& _privateKey, bcos::bytes& _blindResult)
    {
        auto hashResult = m_hashImpl->hash(bcos::bytesConstRef(_input.data(), _input.size()));
        auto ecPoint = m_eccCrypto->hashToCurve(std::move(hashResult));
        _blindResult = m_eccCrypto->ecMultiply(ecPoint, _privateKey);
    }

    virtual void finalizeImpl(bcos::bytes const& _invPrivateKey, bcos::bytes const& _evaluatedData,
        bcos::bytes& _finalizedResult)
    {
        // recover the plainData
        _finalizedResult = m_eccCrypto->ecMultiply(_evaluatedData, _invPrivateKey);
    }
    virtual void fullEvaluateImpl(bcos::bytes const& _input, bcos::bytes& _result)
    {
        auto hashResult = m_hashImpl->hash(
            bcos::bytesConstRef((const unsigned char*)_input.data(), _input.size()));
        auto ecPoint = m_eccCrypto->hashToCurve(std::move(hashResult));
        _result = m_eccCrypto->ecMultiply(ecPoint, m_privateKey);
    }

    virtual void evaluateImpl(bcos::bytes const& _blindData, bcos::bytes& _result)
    {
        _result = m_eccCrypto->ecMultiply(_blindData, m_privateKey);
    }

protected:
    bcos::bytes m_privateKey;
    EccCrypto::Ptr m_eccCrypto;
    Hash::Ptr m_hashImpl;
};
}  // namespace ppc::crypto