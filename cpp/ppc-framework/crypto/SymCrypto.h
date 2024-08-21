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
 * @file SymCrypto.h
 * @author: shawnhe
 * @date 2022-11-29
 */

#pragma once

#include "../Common.h"
#include "../libwrapper/Buffer.h"
#include "../protocol/Protocol.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <memory>

namespace ppc::crypto
{
class SymCrypto
{
public:
    using Ptr = std::shared_ptr<SymCrypto>;
    static constexpr uint32_t DEFAULT_BLOCK_SIZE_BYTE = 16;
    enum class OperationMode : int
    {
        ECB,
        CBC,
        CFB,
        OFB,
        CTR,
    };
    SymCrypto() : m_blockSize(16) {}
    virtual ~SymCrypto() = default;


    virtual ppc::protocol::SymCryptoImplName type() const = 0;

    virtual bcos::bytes generateKey(OperationMode _mode) const = 0;
    virtual void generateKey(OutputBuffer* _key, OperationMode _mode) const = 0;

    virtual bcos::bytes encrypt(OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _plaintext) const = 0;
    virtual void encrypt(OutputBuffer* _cipher, OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _plaintext) const = 0;

    virtual void decrypt(OutputBuffer* plain, OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _ciphertext) const = 0;

    virtual bcos::bytes decrypt(OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _ciphertext) const = 0;

    virtual unsigned int keyBytes(OperationMode _mode) const = 0;
    unsigned int blockSize() const { return m_blockSize; }

protected:
    // the length of the block
    unsigned int m_blockSize = DEFAULT_BLOCK_SIZE_BYTE;
};

}  // namespace ppc::crypto
