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
 * @file BLAKE2bPRNG.h
 * @author: shawnhe
 * @date 2022-12-4
 */

#pragma once
#include "ppc-crypto-core/src/hash/BLAKE2bHash.h"
#include "ppc-framework/crypto/PRNG.h"

namespace ppc::crypto
{
class BLAKE2bPRNG : public PRNG
{
public:
    using Ptr = std::shared_ptr<BLAKE2bPRNG>;
    static constexpr uint32_t BUFFER_CAPACITY = 64;

    // default constructor is not allowed
    BLAKE2bPRNG() = delete;
    virtual ~BLAKE2bPRNG() = default;

    BLAKE2bPRNG(const bcos::bytes& _seed) : PRNG(_seed)
    {
        m_blake2Hash = std::make_shared<BLAKE2bHash>();
        bcos::bytes index(BUFFER_CAPACITY);
        m_buffer = m_blake2Hash->hash(bcos::ref(index), bcos::ref(m_seeds), BUFFER_CAPACITY);
    }

    ppc::protocol::PRNGImplName type() const override
    {
        return ppc::protocol::PRNGImplName::BLAKE2b;
    }

    // generate `_bytesLen` bytes random numbers, saved in _dest
    void generate(bcos::byte* _dest, uint64_t _bytesLen) override;

private:
    void parallelGen(bcos::byte* _dest, uint64_t _currentRound, uint64_t _remainRound);

    BLAKE2bHash::Ptr m_blake2Hash;

    // record current buffer
    bcos::bytes m_buffer;
};

}  // namespace ppc::crypto
