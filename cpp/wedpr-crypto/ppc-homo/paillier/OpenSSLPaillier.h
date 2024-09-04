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
 * @file OpenSSLPaillier.h
 * @author: yujiechen
 * @date 2023-08-04
 */
#pragma once
#include "../codec/SignedNumberCodec.h"
#include "Common.h"
#include "OpenSSLPaillierKeyPair.h"
#include <ppc-framework/crypto/Paillier.h>

namespace ppc::homo
{
class OpenSSLPaillier : public Paillier
{
public:
    using Ptr = std::shared_ptr<OpenSSLPaillier>;
    OpenSSLPaillier() = default;
    virtual ~OpenSSLPaillier() = default;

    // generate the keypair
    ppc::crypto::KeyPair::UniquePtr generateKeyPair(unsigned const _keyLength) const override;

    // encrypt the plain data
    void encrypt_with_crt(OutputBuffer* _resultBuffer, OutputBuffer* _rBuffer, BIGNUM const* v,
        void* _keyPair) const override;

    bcos::bytes encrypt_with_crt(BIGNUM const* _value, void* _keyPair) const override
    {
        auto keyPair = (OpenSSLPaillierKeyPair*)_keyPair;
        if (!keyPair || !keyPair->sk() || !keyPair->pk())
        {
            BOOST_THROW_EXCEPTION(InvalidHomoKeyPair());
        }
        auto pk = (PaillierPublicKey*)keyPair->pk();
        bcos::bytes result(maxCipherBytesLen(pk->keyBits), 0);
        OutputBuffer buffer{result.data(), result.size()};
        encrypt_with_crt(&buffer, nullptr, _value, keyPair);
        result.resize(buffer.len);
        return result;
    }

    void encrypt(OutputBuffer* _result, OutputBuffer* _rBuffer, BIGNUM const* _value,
        void* _pk) const override
    {
        encryptImpl(_result, _rBuffer, _value, nullptr, _pk);
    }
    bcos::bytes encrypt(BIGNUM const* _value, void* _pk) const override
    {
        if (!_value)
        {
            BOOST_THROW_EXCEPTION(OpenSSLPaillierException()
                                  << bcos::errinfo_comment("encrypt error for invalid input v"));
        }
        if (!_pk)
        {
            BOOST_THROW_EXCEPTION(OpenSSLPaillierException()
                                  << bcos::errinfo_comment("encrypt error for invalid public key"));
        }
        auto pk = (PaillierPublicKey*)_pk;
        bcos::bytes result(maxCipherBytesLen(pk->keyBits), 0);
        OutputBuffer buffer{result.data(), result.size()};
        encrypt(&buffer, nullptr, _value, _pk);
        result.resize(buffer.len);
        return result;
    }

    // decrypt the cipher data
    ppc::crypto::BigNum decrypt(
        bcos::bytesConstRef const& _cipherData, void* _keyPair) const override;

    // _result  = _cipher1 + _cipher2
    void add(OutputBuffer* _result, bcos::bytesConstRef const& _cipher1,
        bcos::bytesConstRef const& _cipher2, void* _publicKey) const override;

    bcos::bytes add(bcos::bytesConstRef const& _cipher1, bcos::bytesConstRef const& _cipher2,
        void* _publicKey) const override
    {
        if (!_publicKey)
        {
            BOOST_THROW_EXCEPTION(InvalidHomoPublicKey());
        }
        auto pk = (PaillierPublicKey*)_publicKey;
        bcos::bytes result(maxCipherBytesLen(pk->keyBits), 0);
        OutputBuffer buffer{result.data(), result.size()};
        add(&buffer, _cipher1, _cipher2, _publicKey);
        result.resize(buffer.len);
        return result;
    }

    // _result = _cipher1 - _cipher2
    void sub(OutputBuffer* _result, bcos::bytesConstRef const& _cipher1,
        bcos::bytesConstRef const& _cipher2, void* _publicKey) const override;

    bcos::bytes sub(bcos::bytesConstRef const& _cipher1, bcos::bytesConstRef const& _cipher2,
        void* _publicKey) const override
    {
        if (!_publicKey)
        {
            BOOST_THROW_EXCEPTION(InvalidHomoPublicKey());
        }
        auto pk = (PaillierPublicKey*)_publicKey;
        bcos::bytes result(maxCipherBytesLen(pk->keyBits), 0);

        OutputBuffer buffer{result.data(), result.size()};
        sub(&buffer, _cipher1, _cipher2, _publicKey);
        result.resize(buffer.len);
        return result;
    }

    // _result = _v * _cipher
    void scalaMul(OutputBuffer* _result, BIGNUM const* _v, bcos::bytesConstRef const& _cipher,
        void* _publicKey) const override;

    bcos::bytes scalaMul(
        BIGNUM const* v, bcos::bytesConstRef const& _cipher, void* _publicKey) const override
    {
        if (!_publicKey)
        {
            BOOST_THROW_EXCEPTION(InvalidHomoPublicKey());
        }
        auto pk = (PaillierPublicKey*)_publicKey;
        bcos::bytes result(maxCipherBytesLen(pk->keyBits), 0);

        OutputBuffer buffer{result.data(), result.size()};
        scalaMul(&buffer, v, _cipher, _publicKey);
        result.resize(buffer.len);

        return result;
    }

    // calculate the cipherBytesLen
    static unsigned int maxCipherBytesLen(unsigned int _keyBits)
    {
        // 1(the signed/unsigned flag) + _keyBits * 2/ 8
        return ((_keyBits) >> 2) + 1;
    }

    static unsigned int rBytesLen(unsigned int _keyBits)
    {
        auto rBits = _keyBits % 2 ? ((_keyBits / 2) + 1) : (_keyBits / 2);
        return (rBits + 7) >> 3;
    }

protected:
    virtual void encryptImpl(OutputBuffer* _resultBuffer, OutputBuffer* _rBuffer, BIGNUM const* m,
        OpenSSLPaillierKeyPair* _keyPair, void* _pk) const;

    void addImpl(OutputBuffer* _result, ppc::crypto::BigNum const& _cipher1,
        ppc::crypto::BigNum const& _cipher2, void* _publicKey) const;

private:
    ppc::crypto::BigNum powerModSqrtCrt(ppc::crypto::BigNum const& _base,
        ppc::crypto::BigNum const& _exp, OpenSSLPaillierKeyPair* _keyPair) const;
};
}  // namespace ppc::homo