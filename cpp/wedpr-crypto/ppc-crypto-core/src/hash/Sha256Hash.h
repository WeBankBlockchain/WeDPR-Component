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
 * @file Sha256Hash.h
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
class Sha256HashState : public HashState
{
public:
    using Ptr = std::shared_ptr<Sha256HashState>;
    Sha256HashState() : m_state(std::make_shared<crypto_hash_sha256_state>()) {}
    ~Sha256HashState() override = default;
    void* state() override { return (void*)m_state.get(); }

private:
    std::shared_ptr<crypto_hash_sha256_state> m_state;
};

class Sha256Hash : public Hash
{
public:
    using Ptr = std::shared_ptr<Sha256Hash>;
    Sha256Hash() = default;
    ~Sha256Hash() override = default;

    // the hashBytes length of given hash-algorithm
    size_t hashLen() const override { return crypto_hash_sha256_BYTES; }
    // the implementation of the hash-algorithm
    ppc::protocol::HashImplName type() const override
    {
        return ppc::protocol::HashImplName::SHA256;
    }

    // calculate-hash
    bcos::bytes hash(bcos::bytesConstRef _input) const override
    {
        bcos::bytes result(crypto_hash_sha256_BYTES);
        auto ret = crypto_hash_sha256(result.data(), _input.data(), _input.size());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha256 error: " + std::to_string(ret)));
        }
        return result;
    }

    // init the hash-state
    HashState::Ptr init() const override
    {
        auto state = std::make_shared<Sha256HashState>();
        auto ret = crypto_hash_sha256_init((crypto_hash_sha256_state*)state->state());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha256_init error: " + std::to_string(ret)));
        }
        return state;
    }

    // update new message into the given hash-state
    void update(HashState::Ptr _state, bcos::bytesConstRef _data) const override
    {
        auto ret = crypto_hash_sha256_update(
            (crypto_hash_sha256_state*)_state->state(), _data.data(), _data.size());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha256_update error: " + std::to_string(ret)));
        }
    }

    // obtain the hash-result from the given hash-state
    bcos::bytes final(HashState::Ptr _state) const override
    {
        bcos::bytes result(crypto_hash_sha256_BYTES);
        auto ret =
            crypto_hash_sha256_final((crypto_hash_sha256_state*)_state->state(), result.data());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "crypto_hash_sha256_final error: " + std::to_string(ret)));
        }
        return result;
    }
};
}  // namespace ppc::crypto