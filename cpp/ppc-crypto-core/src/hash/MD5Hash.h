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
 * @file MD5Hash.h
 * @author: yujiechen
 * @date 2022-11-10
 */
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "../Common.h"
#include "openssl/md5.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/Common.h>

namespace ppc::crypto
{
class MD5HashState : public HashState
{
public:
    using Ptr = std::shared_ptr<MD5HashState>;
    MD5HashState() : m_state(std::make_shared<MD5state_st>()) {}
    ~MD5HashState() override = default;

    void* state() override { return (void*)m_state.get(); }

private:
    std::shared_ptr<MD5state_st> m_state;
};

class MD5Hash : public Hash
{
public:
    using Ptr = std::shared_ptr<MD5Hash>;
    MD5Hash() = default;
    ~MD5Hash() override = default;

    // the hashBytes length of given hash-algorithm
    size_t hashLen() const override { return MD5_DIGEST_LENGTH; }
    // the implementation of the hash-algorithm
    ppc::protocol::HashImplName type() const override { return ppc::protocol::HashImplName::MD5; }

    // calculate-hash
    bcos::bytes hash(bcos::bytesConstRef _input) const override
    {
        bcos::bytes result(MD5_DIGEST_LENGTH);
        MD5(_input.data(), _input.size(), result.data());
        return result;
    }

    // init the hash-state
    HashState::Ptr init() const override
    {
        auto hashState = std::make_shared<MD5HashState>();
        auto ret = MD5_Init((MD5_CTX*)hashState->state());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(
                HashException() << bcos::errinfo_comment("md5_init error: " + std::to_string(ret)));
        }
        return hashState;
    }

    // update new message into the given hash-state
    void update(HashState::Ptr _state, bcos::bytesConstRef _data) const override
    {
        auto ret = MD5_Update((MD5_CTX*)_state->state(), (const void*)_data.data(), _data.size());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "md5_update error: " + std::to_string(ret)));
        }
    }

    // obtain the hash-result from the given hash-state
    bcos::bytes final(HashState::Ptr _state) const override
    {
        bcos::bytes result(MD5_DIGEST_LENGTH);
        auto ret = MD5_Final(result.data(), (MD5_CTX*)_state->state());
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(HashException() << bcos::errinfo_comment(
                                      "md5_final error: " + std::to_string(ret)));
        }
        return result;
    }
};
}  // namespace ppc::crypto
#pragma GCC diagnostic pop