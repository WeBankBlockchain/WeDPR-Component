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
 * @file EcdhOprf.h
 * @author: shawnhe
 * @date 2022-11-3
 */

#pragma once
#include "ppc-framework/crypto/EccCrypto.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/crypto/Oprf.h"

namespace ppc::crypto
{
/**
 *
 * reference: JKK14
 * title: Round-optimal password-protected secret sharing and T-PAKE in the password-only model
 * url: https://eprint.iacr.org/2014/650.pdf
 *
 * OPRF(x, sk) = H2(x, H1(x)^sk)
 *
 * Client: y, r                                                            Server: x, sk
 * -------------------------------------------------------------------------------------
 * blindedElement = H1(y)^r
 *
 *                                   blindedElement
 *                                   ---------->
 *
 *                                   evaluatedElement = H1(y)^r)^sk
 *
 *                                   evaluatedElement
 *                                   <----------
 *
 * output = H2(y,(H1(y)^r)^sk^(1/r))=H2(y,H1(y)^sk)
 */

class EcdhOprfClient : public OprfClient
{
public:
    using Ptr = std::shared_ptr<EcdhOprfClient>;
    EcdhOprfClient() = delete;
    EcdhOprfClient(uint16_t _outputSize, Hash::Ptr _hash, EccCrypto::Ptr _eccCrypto)
      : OprfClient(ppc::protocol::OprfType::EcdhOprf, _outputSize),
        m_hash(std::move(_hash)),
        m_eccCrypto(std::move(_eccCrypto))
    {
        m_type = protocol::OprfType::EcdhOprf;

        // init private key
        m_privateKey = m_eccCrypto->generateRandomScalar();
    }

    virtual ~EcdhOprfClient() = default;

    // blind input by private key
    bcos::bytes blind(const bcos::bytes& _input) const override;
    bcos::bytes blind(std::string_view _input) const override;
    std::vector<bcos::bytes> blind(const std::vector<std::string>& _inputs) const override;

    // unblind evaluated item, and hash with input
    bcos::bytes finalize(std::string_view _input, const bcos::bytes& _evaluatedItem) const override;
    std::vector<bcos::bytes> finalize(const std::vector<std::string>& _inputs,
        const std::vector<bcos::bytes>& _evaluatedItems) const override;

private:
    Hash::Ptr m_hash;
    EccCrypto::Ptr m_eccCrypto;
};

class EcdhOprfServer : public OprfServer
{
public:
    using Ptr = std::shared_ptr<EcdhOprfServer>;
    EcdhOprfServer() = delete;
    EcdhOprfServer(uint16_t _outputSize, Hash::Ptr _hash, EccCrypto::Ptr _eccCrypto)
      : OprfServer(ppc::protocol::OprfType::EcdhOprf, _outputSize),
        m_hash(std::move(_hash)),
        m_eccCrypto(std::move(_eccCrypto))
    {
        m_type = protocol::OprfType::EcdhOprf;

        // init private key
        m_privateKey = m_eccCrypto->generateRandomScalar();
    }

    virtual ~EcdhOprfServer() = default;

    // evaluate blinded item by private key
    bcos::bytes evaluate(const bcos::bytes& _blindedItem) const override;
    std::vector<bcos::bytes> evaluate(const std::vector<bcos::bytes>& _blindedItems) const override;

    // compute the OPRF output with server's private key
    bcos::bytes fullEvaluate(const bcos::bytes& _input) const override;
    bcos::bytes fullEvaluate(const std::string_view _input) const override;
    std::vector<bcos::bytes> fullEvaluate(const std::vector<std::string>& _inputs) const override;

private:
    Hash::Ptr m_hash;
    EccCrypto::Ptr m_eccCrypto;
};
}  // namespace ppc::crypto