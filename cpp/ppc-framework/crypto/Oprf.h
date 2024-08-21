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
 * @file Oprf.h
 * @author: shawnhe
 * @date 2022-11-3
 */

#pragma once
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::crypto
{
/**
 * reference: draft-irtf-cfrg-voprf-14
 * title: Oblivious Pseudorandom Functions (OPRFs) using Prime-Order Groups
 * url: https://datatracker.ietf.org/doc/draft-irtf-cfrg-voprf/
 *
 * In the OPRF mode, a client and server interact to compute output = F(skS, input), where input is
 * the client's private input, skS is the server's private key, and output is the OPRF output. After
 * the execution of the protocol, the client learns output and the server learns nothing.  This
 * interaction is shown below.
 *
 * Client                                                                    Server(sk)
 * -------------------------------------------------------------------------------------
 * blindedElement = Blind(input)
 *
 *                                   blindedElement
 *                                   ---------->
 *
 *                                   evaluatedElement = BlindEvaluate(sk, blindedElement)
 *
 *                                   evaluatedElement
 *                                   <----------
 *
 * output = Finalize(input, blind, evaluatedElement)
 */
class Oprf
{
public:
    using Ptr = std::shared_ptr<Oprf>;
    Oprf() = default;
    Oprf(ppc::protocol::OprfType _type, uint16_t _outputSize)
      : m_type(_type), m_outputSize(_outputSize)
    {}
    virtual ~Oprf() = default;

    ppc::protocol::OprfType type() { return m_type; }
    uint16_t outputSize() { return m_outputSize; }
    void setOutputSize(uint16_t _size) { m_outputSize = _size; }
    bcos::bytes privateKey() { return m_privateKey; }
    void setPrivateKey(bcos::bytes _key) { m_privateKey = std::move(_key); }

protected:
    ppc::protocol::OprfType m_type;
    uint16_t m_outputSize;
    bcos::bytes m_privateKey;
};

class OprfClient : public Oprf
{
public:
    using Ptr = std::shared_ptr<OprfClient>;
    OprfClient() = default;
    OprfClient(ppc::protocol::OprfType _type, uint16_t _outputSize) : Oprf(_type, _outputSize) {}
    virtual ~OprfClient() = default;

    // blind input by private key
    virtual bcos::bytes blind(const bcos::bytes& _input) const = 0;
    virtual bcos::bytes blind(std::string_view _input) const = 0;
    virtual std::vector<bcos::bytes> blind(const std::vector<std::string>& _inputs) const = 0;

    // unblind evaluated item, and hash with input
    virtual bcos::bytes finalize(
        std::string_view _input, const bcos::bytes& _evaluatedItem) const = 0;
    virtual std::vector<bcos::bytes> finalize(const std::vector<std::string>& _inputs,
        const std::vector<bcos::bytes>& _evaluatedItems) const = 0;
};


class OprfServer : public Oprf
{
public:
    using Ptr = std::shared_ptr<OprfServer>;
    OprfServer() = default;
    OprfServer(ppc::protocol::OprfType _type, uint16_t _outputSize) : Oprf(_type, _outputSize) {}
    virtual ~OprfServer() = default;

    // evaluate blinded item by private key
    virtual bcos::bytes evaluate(const bcos::bytes& _blindedItem) const = 0;
    virtual std::vector<bcos::bytes> evaluate(
        const std::vector<bcos::bytes>& _blindedItems) const = 0;

    // compute the OPRF output with server's private key
    virtual bcos::bytes fullEvaluate(const bcos::bytes& _input) const = 0;
    virtual bcos::bytes fullEvaluate(const std::string_view _input) const = 0;
    virtual std::vector<bcos::bytes> fullEvaluate(
        const std::vector<std::string>& _inputs) const = 0;
};

}  // namespace ppc::crypto