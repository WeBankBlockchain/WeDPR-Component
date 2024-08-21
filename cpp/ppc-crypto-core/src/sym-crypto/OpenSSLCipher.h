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
 * @file OpenSSLCipher.h
 * @author: shawnhe
 * @date 2023-08-17
 */

#pragma once

#include "../Common.h"
#include "ppc-framework/crypto/SymCrypto.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include <openssl/evp.h>

namespace ppc::crypto
{
struct EvpCipherDeleter
{
public:
    void operator()(EVP_CIPHER const* _cipher) { (void)_cipher; }
};
using EvpCipherPtr = std::shared_ptr<const EVP_CIPHER>;

struct EvpCipherCtxDeleter
{
public:
    void operator()(EVP_CIPHER_CTX* _ctx) { EVP_CIPHER_CTX_free(_ctx); }
};
using EvpCipherCtxPtr = std::shared_ptr<EVP_CIPHER_CTX>;

class OpenSSLCipher : public SymCrypto
{
public:
    OpenSSLCipher() = delete;

    ~OpenSSLCipher() override = default;

    OpenSSLCipher(protocol::DataPaddingType _padding = protocol::DataPaddingType::PKCS7)
      : SymCrypto()
    {
        m_padding = _padding;
        if (int(m_padding) < 0 || int(m_padding) > EVP_PADDING_ZERO)
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "undefined padding: " + std::to_string(int(_padding))));
        }
    }

private:
    virtual EvpCipherPtr createCipherMeth(OperationMode _mode) const = 0;

    virtual void initCipherCtx(OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, EVP_CIPHER_CTX* _ctx, int _enc) const
    {
        auto cipher = createCipherMeth(_mode);
        auto keyLength = EVP_CIPHER_key_length(cipher.get());
        auto key = _sk.toBytes();
        key.resize(keyLength);
        auto iv = _iv.toBytes();
        iv.resize(m_blockSize);
        if (_mode == SymCrypto::OperationMode::ECB)
        {
            if (1 != EVP_CipherInit_ex(_ctx, cipher.get(), nullptr, key.data(), nullptr, _enc))
            {
                BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                          "initCipherCtx: EVP_CipherInit_ex failed: " +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
            return;
        }

        if (1 != EVP_CipherInit_ex(_ctx, cipher.get(), nullptr, key.data(), iv.data(), _enc))
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "initCipherCtx: EVP_CipherInit_ex failed: " +
                                      std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
    }

    virtual EvpCipherCtxPtr createCipherCtx() const
    {
        return EvpCipherCtxPtr(EVP_CIPHER_CTX_new(), EvpCipherCtxDeleter());
    }

public:
    protocol::DataPaddingType padding() { return m_padding; }

    void encrypt(OutputBuffer* _cipher, OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _plaintext) const override
    {
        auto minCipherSize = _plaintext.size() + m_blockSize;
        if (_cipher->len < minCipherSize)
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "The reserved cipher buffer is not enough, at least: " +
                                      std::to_string(minCipherSize)));
        }
        auto ctx = createCipherCtx();
        initCipherCtx(_mode, _sk, _iv, ctx.get(), 1);

        if (_mode == SymCrypto::OperationMode::ECB || _mode == SymCrypto::OperationMode::CBC)
        {
            EVP_CIPHER_CTX_set_padding(ctx.get(), int(m_padding));
        }
        EVP_CIPHER_CTX_set_padding(ctx.get(), int(m_padding));
        int outLen;
        if (1 != EVP_CipherUpdate(
                     ctx.get(), _cipher->data, &outLen, _plaintext.data(), _plaintext.size()))
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "encrypt failed for EVP_CipherUpdate exception: " +
                                      std::string(ERR_error_string(ERR_get_error(), NULL))));
        }

        int outLenFinal = 0;
        if (1 != EVP_CipherFinal_ex(ctx.get(), _cipher->data + outLen, &outLenFinal))
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "encrypt failed for EVP_CipherFinal exception: " +
                                      std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
        _cipher->len = outLen + outLenFinal;
    }

    bcos::bytes encrypt(OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _plaintext) const override
    {
        bcos::bytes cipherResult(_plaintext.size() + blockSize());
        OutputBuffer cipherBuffer{cipherResult.data(), cipherResult.size()};
        encrypt(&cipherBuffer, _mode, _sk, _iv, _plaintext);
        cipherResult.resize(cipherBuffer.len);
        return cipherResult;
    }

    void decrypt(OutputBuffer* plain, OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _ciphertext) const override
    {
        auto minPlainDataSize = _ciphertext.size() + m_blockSize;
        if (plain->len < minPlainDataSize)
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "The reserved plainData buffer is not enough, at least: " +
                                      std::to_string(minPlainDataSize)));
        }
        auto ctx = createCipherCtx();
        initCipherCtx(_mode, _sk, _iv, ctx.get(), 0);
        if (_mode == SymCrypto::OperationMode::ECB || _mode == SymCrypto::OperationMode::CBC)
        {
            EVP_CIPHER_CTX_set_padding(ctx.get(), int(m_padding));
        }
        EVP_CIPHER_CTX_set_padding(ctx.get(), int(m_padding));

        int outLen;
        if (1 != EVP_CipherUpdate(
                     ctx.get(), plain->data, &outLen, _ciphertext.data(), _ciphertext.size()))
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "decrypt failed for EVP_CipherUpdate exception: " +
                                      std::string(ERR_error_string(ERR_get_error(), NULL))));
        }

        int outLenFinal = 0;
        if (1 != EVP_CipherFinal_ex(ctx.get(), plain->data + outLen, &outLenFinal))
        {
            BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                      "decrypt failed for EVP_CipherFinal exception: " +
                                      std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
        plain->len = outLen + outLenFinal;
    }

    bcos::bytes decrypt(OperationMode _mode, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _iv, bcos::bytesConstRef const& _ciphertext) const override
    {
        bcos::bytes result(_ciphertext.size() + m_blockSize, 0);
        OutputBuffer resultBuffer{result.data(), result.size()};
        decrypt(&resultBuffer, _mode, _sk, _iv, _ciphertext);
        result.resize(resultBuffer.len);
        return result;
    }

    bcos::bytes generateKey(OperationMode _mode) const override
    {
        auto key = generateRand(keyBytes(_mode) * 8);
        bcos::bytes keyBytes;
        key.toBytes(keyBytes, false);
        return keyBytes;
    }

    void generateKey(OutputBuffer* _key, OperationMode _mode) const override
    {
        auto key = generateRand(keyBytes(_mode) * 8);
        key.toOutputBuffer(_key, false);
    }

    virtual unsigned int keyBytes(OperationMode _mode) const override
    {
        auto cipher = createCipherMeth(_mode);
        return EVP_CIPHER_key_length(cipher.get());
    }

protected:
    protocol::DataPaddingType m_padding{protocol::DataPaddingType::PKCS7};
};
}  // namespace ppc::crypto
