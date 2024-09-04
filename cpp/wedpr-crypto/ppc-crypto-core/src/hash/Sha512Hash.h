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
 * @file Sha512Hash.h
 * @author: yujiechen
 * @date 2022-11-2
 */
#pragma once
#include "../Common.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/protocol/Protocol.h"
#include <sodium.h>

namespace ppc::crypto
{
class Sha512HashState : public HashState
{
public:
    using Ptr = std::shared_ptr<Sha512HashState>;
    Sha512HashState() : m_state(std::make_shared<crypto_hash_sha512_state>()) {}
    ~Sha512HashState() override = default;
    void* state() override { return (void*)m_state.get(); }

private:
    std::shared_ptr<crypto_hash_sha512_state> m_state;
};

class Sha512Hash : public Hash
{
public:
    using Ptr = std::shared_ptr<Sha512Hash>;
    Sha512Hash() = default;
    virtual ~Sha512Hash() = default;

    // the hashBytes length of given hash-algorithm
    size_t hashLen() const override { return crypto_hash_sha512_BYTES; }
    // the implementation of the hash-algorithm
    ppc::protocol::HashImplName type() const override
    {
        return ppc::protocol::HashImplName::SHA512;
    }

    // calculate-hash
    bcos::bytes hash(bcos::bytesConstRef _input) const override
    {
        bcos::bytes result(crypto_hash_sha512_BYTES);
        auto ret = crypto_hash_sha512(result.data(), _input.data(), _input.size());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha512 error: " + std::to_string(ret)));
        }
        return result;
    }

    // init the hash-state
    HashState::Ptr init() const override
    {
        auto hashState = std::make_shared<Sha512HashState>();
        auto ret = crypto_hash_sha512_init((crypto_hash_sha512_state*)hashState->state());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha512_init error: " + std::to_string(ret)));
        }
        return hashState;
    }

    // update new message into the given hash-state
    void update(HashState::Ptr _state, bcos::bytesConstRef _data) const override
    {
        auto ret = crypto_hash_sha512_update(
            (crypto_hash_sha512_state*)_state->state(), _data.data(), _data.size());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha512_state error: " + std::to_string(ret)));
        }
    }

    // obtain the hash-result from the given hash-state
    bcos::bytes final(HashState::Ptr _state) const override
    {
        bcos::bytes result(crypto_hash_sha512_BYTES);
        auto ret =
            crypto_hash_sha512_final((crypto_hash_sha512_state*)_state->state(), result.data());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha512_final error: " + std::to_string(ret)));
        }
        return result;
    }
};
}  // namespace ppc::crypto