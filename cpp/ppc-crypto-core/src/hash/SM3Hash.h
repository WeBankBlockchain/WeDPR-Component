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
 * @file SM3Hash.h
 * @author: yujiechen
 * @date 2022-11-2
 */
#pragma once
#include "../Common.h"
#include "openssl/sm3.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/protocol/Protocol.h"

namespace ppc::crypto
{
class SM3HashState : public HashState
{
public:
    using Ptr = std::shared_ptr<SM3HashState>;
    SM3HashState() : m_state(std::make_shared<SM3_CTX>()) {}
    ~SM3HashState() override = default;

    void* state() override { return (void*)m_state.get(); }

private:
    std::shared_ptr<SM3_CTX> m_state;
};

class SM3Hash : public Hash
{
public:
    using Ptr = std::shared_ptr<SM3Hash>;
    SM3Hash() = default;
    ~SM3Hash() override = default;

    // the hashBytes length of given hash-algorithm
    size_t hashLen() const override { return 32; }
    // the implementation of the hash-algorithm
    ppc::protocol::HashImplName type() const override { return ppc::protocol::HashImplName::SM3; }

    // calculate-hash
    bcos::bytes hash(bcos::bytesConstRef _input) const override
    {
        SM3_CTX hashState;
        auto ret = sm3_init(&hashState);
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(
                HashException() << bcos::errinfo_comment(
                    "hash error for sm3_init failed, code: " + std::to_string(ret)));
        }
        ret = sm3_update(&hashState, (const void*)_input.data(), _input.size());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(
                HashException() << bcos::errinfo_comment(
                    "hash error for sm3_update failed, code: " + std::to_string(ret)));
        }
        bcos::bytes result(32);
        ret = sm3_final(result.data(), &hashState);
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(
                HashException() << bcos::errinfo_comment(
                    "hash error for sm3_final failed, code: " + std::to_string(ret)));
        }
        return result;
    }

    // init the hash-state
    HashState::Ptr init() const override
    {
        auto hashState = std::make_shared<SM3HashState>();
        auto ret = sm3_init((SM3_CTX*)hashState->state());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(
                HashException() << bcos::errinfo_comment("sm3_init error: " + std::to_string(ret)));
        }
        return hashState;
    }

    // update new message into the given hash-state
    void update(HashState::Ptr _state, bcos::bytesConstRef _data) const override
    {
        auto ret = sm3_update((SM3_CTX*)_state->state(), (const void*)_data.data(), _data.size());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "sm3_update error: " + std::to_string(ret)));
        }
    }

    // obtain the hash-result from the given hash-state
    bcos::bytes final(HashState::Ptr _state) const override
    {
        bcos::bytes result(32);
        auto ret = sm3_final(result.data(), (SM3_CTX*)_state->state());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "sm3_final error: " + std::to_string(ret)));
        }
        return result;
    }
};
}  // namespace ppc::crypto
