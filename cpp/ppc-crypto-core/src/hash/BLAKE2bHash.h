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
 * @file BLAKE2bHash.h
 * @author: shawnhe
 * @date 2022-12-4
 */

#pragma once
#include "../Common.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/protocol/Protocol.h"
#include <sodium.h>

namespace ppc::crypto
{
class BLAKE2bHashState : public HashState
{
public:
    using Ptr = std::shared_ptr<BLAKE2bHashState>;
    BLAKE2bHashState() : m_state(std::make_shared<crypto_generichash_state>()) {}
    ~BLAKE2bHashState() override = default;

    void* state() override { return (void*)m_state.get(); }

private:
    std::shared_ptr<crypto_generichash_state> m_state;
};

class BLAKE2bHash : public Hash
{
public:
    using Ptr = std::shared_ptr<BLAKE2bHash>;
    BLAKE2bHash() = default;
    ~BLAKE2bHash() override = default;

    // the hashBytes length of given hash-algorithm
    size_t hashLen() const override { return crypto_generichash_BYTES_MAX; }

    // the implementation of the hash-algorithm
    ppc::protocol::HashImplName type() const override
    {
        return ppc::protocol::HashImplName::BLAKE2b;
    }

    // calculate hash and output default length
    bcos::bytes hash(bcos::bytesConstRef _input) const override
    {
        return hash(_input, crypto_generichash_BYTES_MAX);
    }

    // calculate hash without key
    bcos::bytes hash(bcos::bytesConstRef _input, size_t _outLen) const
    {
        return hash(_input, bcos::bytesConstRef(), _outLen);
    }

    // calculate hash with a key
    bcos::bytes hash(bcos::bytesConstRef _input, bcos::bytesConstRef _key, size_t _outLen) const
    {
        if (_outLen < crypto_generichash_BYTES_MIN || _outLen > crypto_generichash_BYTES_MAX)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "outLen of BLAKE2b should be between " +
                                      std::to_string(crypto_generichash_BYTES_MIN) + " and " +
                                      std::to_string(crypto_generichash_BYTES_MAX)));
        }

        bcos::bytes result(_outLen);
        auto ret = crypto_generichash(result.data(), _outLen, (const unsigned char*)_input.data(),
            _input.size(), (const unsigned char*)_key.data(), _key.size());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(
                HashException() << bcos::errinfo_comment(
                    "hash error for BLAKE2b_hash failed, code: " + std::to_string(ret)));
        }
        return result;
    }

    // init the hash-state
    HashState::Ptr init() const override { return init(bcos::bytesConstRef()); }

    // init the hash-state with a key
    HashState::Ptr init(bcos::bytesConstRef _key) const
    {
        auto state = std::make_shared<BLAKE2bHashState>();
        auto ret = crypto_generichash_init((crypto_generichash_state*)state->state(),
            (const unsigned char*)_key.data(), _key.size(), crypto_generichash_BYTES_MAX);
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "BLAKE2b_init error: " + std::to_string(ret)));
        }
        return state;
    }

    // update new message into the given hash-state
    void update(HashState::Ptr _state, bcos::bytesConstRef _data) const override
    {
        auto ret = crypto_generichash_update((crypto_generichash_state*)_state->state(),
            (const unsigned char*)_data.data(), _data.size());
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "BLAKE2b_update error: " + std::to_string(ret)));
        }
    }

    // obtain the hash-result from the given hash-state
    bcos::bytes final(HashState::Ptr _state) const override
    {
        bcos::bytes result(crypto_generichash_BYTES_MAX);
        auto ret = crypto_generichash_final((crypto_generichash_state*)_state->state(),
            result.data(), crypto_generichash_BYTES_MAX);
        if (ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "BLAKE2b_final error: " + std::to_string(ret)));
        }
        return result;
    }
};
}  // namespace ppc::crypto
