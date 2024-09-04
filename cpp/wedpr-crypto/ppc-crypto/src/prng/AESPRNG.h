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
 * @file AESPRNG.h
 * @author: shawnhe
 * @date 2022-11-29
 */

#pragma once
#include "ppc-crypto-core/src/sym-crypto/OpenSSLAES.h"
#include "ppc-framework/crypto/PRNG.h"

namespace ppc::crypto
{
class AESPRNG : public PRNG
{
public:
    using Ptr = std::shared_ptr<AESPRNG>;
    static constexpr uint64_t BUFFER_CAPACITY = 1024;

    // default constructor is not allowed
    AESPRNG() = delete;
    virtual ~AESPRNG() = default;

    AESPRNG(const bcos::bytes& _seed) : PRNG(_seed), m_key(_seed), m_iv(bcos::byte())
    {
        m_aes = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128);
        bcos::bytes index(BUFFER_CAPACITY);
        m_buffer = m_aes->encrypt(
            SymCrypto::OperationMode::CBC, bcos::ref(m_key), bcos::ref(m_iv), bcos::ref(index));
    }

    ppc::protocol::PRNGImplName type() const override { return ppc::protocol::PRNGImplName::AES; }

    // generate `_bytesLen` bytes random numbers, saved in _dest
    void generate(bcos::byte* _dest, uint64_t _bytesLen) override;

private:
    void parallelGen(bcos::byte* _dest, uint64_t _currentRound, uint64_t _remainRound);

    OpenSSLAES::Ptr m_aes;
    bcos::bytes m_key;
    bcos::bytes m_iv;

    // record current buffer
    bcos::bytes m_buffer;
};

}  // namespace ppc::crypto
