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
 * @file FastOre.h
 * @author: shawnhe
 * @date 2023-08-18
 */

#pragma once

#include "ppc-crypto-core/src/sym-crypto/OpenSSLAES.h"
#include "ppc-framework/crypto/Ore.h"
#include "ppc-framework/libwrapper/OreFloatingNumber.h"

#define CIPHER_BLOCK_SIZE 2

namespace ppc::crypto
{
class FastOre : public Ore
{
public:
    using Ptr = std::shared_ptr<FastOre>;
    FastOre() { m_symCrypto = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128); }
    ~FastOre() override = default;

    // cipher encoded with base64
    void encrypt4String(OutputBuffer* _cipher, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _plaintext) const override;
    std::string encrypt4String(
        bcos::bytesConstRef const& _key, const std::string& _plaintext) const override;

    // cipher encoded with base64
    std::string decrypt4String(
        bcos::bytesConstRef const& _key, const std::string& _ciphertext) const override;
    void decrypt4String(OutputBuffer* plain, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& cipher) const override;

    // cipher encoded with base64
    std::string encrypt4Integer(
        bcos::bytesConstRef const& _sk, const int64_t& _plain) const override;
    void encrypt4Integer(OutputBuffer* _cipher, bcos::bytesConstRef const& _sk,
        const int64_t& _plain) const override;

    // cipher encoded with base64
    int64_t decrypt4Integer(
        bcos::bytesConstRef const& _sk, const std::string& _cipher) const override;
    void decrypt4Integer(int64_t* _plain, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _cipher) const override;

    // cipher encoded with base64
    std::string encrypt4Float(bcos::bytesConstRef const& _sk, const float50& _plain) const override;
    void encrypt4Float(OutputBuffer* _cipher, bcos::bytesConstRef const& _sk,
        const float50& _plain) const override;

    // cipher encoded with base64
    float50 decrypt4Float(
        bcos::bytesConstRef const& _sk, const std::string& _cipher) const override;
    float50 decrypt4Float(
        bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& _cipher) const override;

    int compare(const std::string& _ciphertext0, const std::string& _ciphertext1) const override;
    int compare(InputBuffer const* c1, InputBuffer const* c2) const override;

    int keyBytes() const override { return m_symCrypto->keyBytes(SymCrypto::OperationMode::CBC); }

    bcos::bytes generateKey() const override
    {
        return m_symCrypto->generateKey(SymCrypto::OperationMode::CBC);
    }

    void generateKey(OutputBuffer* sk) const override
    {
        return m_symCrypto->generateKey(sk, SymCrypto::OperationMode::CBC);
    }


    uint64_t estimatedCipherSize(uint64_t _plainSize, bool hex) const
    {
        if (!hex)
        {
            return _plainSize * CIPHER_BLOCK_SIZE;
        }
        return _plainSize * CIPHER_BLOCK_SIZE * 2;
    }

    uint64_t estimatedFloatCipherSize(uint64_t _plainSize, bool hex) const
    {
        if (!hex)
        {
            return (sizeof(int64_t) + _plainSize) * CIPHER_BLOCK_SIZE;
        }
        return (sizeof(int64_t) + _plainSize) * CIPHER_BLOCK_SIZE * 2;
    }

    uint64_t estimatedPlainSize(uint64_t cipherSize, bool hex) const
    {
        uint64_t plainSize = 0;
        if (!hex)
        {
            plainSize = cipherSize / CIPHER_BLOCK_SIZE;
        }
        else
        {
            plainSize = cipherSize / CIPHER_BLOCK_SIZE / 2;
        }
        return plainSize;
    }

    static void formatNumberPlain(OutputBuffer* _buffer, const int64_t& _plain)
    {
        // Note: Add brackets here to adapt to windows compilation
        if (_plain > (std::numeric_limits<int64_t>::max)() / 2 ||
            _plain <= (std::numeric_limits<int64_t>::min)() / 2)
        {
            BOOST_THROW_EXCEPTION(
                FastOreException() << bcos::errinfo_comment(
                    "plain is too large or too small, must be in range (-2^62, 2^62)"));
        }

        int64_t plain = _plain + (std::numeric_limits<int64_t>::max)() / 2;

        for (int i = sizeof(plain) - 1; i >= 0; --i)
        {
            _buffer->data[i] = static_cast<unsigned char>((plain >> (8 * (7 - i))) & 0xFF);
        }
        _buffer->len = sizeof(plain);
    }

    static int64_t recoverNumberPlain(const OutputBuffer& outputBuffer)
    {
        int64_t result = 0;

        for (uint64_t i = 0; i < outputBuffer.len; ++i)
        {
            result = (result << 8) | outputBuffer.data[i];
        }

        return result - (std::numeric_limits<int64_t>::max)() / 2;
    }

private:
    // keep only the first two bytes
    bcos::bytes encWithTruncation(
        bcos::bytesConstRef const& _key, bcos::bytesConstRef const& _plaintext) const
    {
        bcos::bytes iv(0);
        bcos::bytes cipher =
            m_symCrypto->encrypt(SymCrypto::OperationMode::CBC, _key, bcos::ref(iv), _plaintext);
        uint64_t size = cipher.size();
        if (cipher[size - 2] == 0xFF)
        {
            cipher[size - 2] = 0xFE;
        }

        bcos::bytes result(2);
        result[0] = cipher[size - 2];
        result[1] = cipher[size - 1];
        return result;
    }

private:
    SymCrypto::Ptr m_symCrypto;
};

}  // namespace ppc::crypto
