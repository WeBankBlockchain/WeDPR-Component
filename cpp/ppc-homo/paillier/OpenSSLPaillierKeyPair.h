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
 * @file OpenSSLPaillierKeyPair.h
 * @author: yujiechen
 * @date 2023-08-07
 */
#pragma once

#include "ppc-framework/crypto/KeyPair.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-tools/src/codec/CodecUtility.h"

namespace ppc::homo
{
///////////// Paillier-DJN /////////////
/// the paillier public key
class PaillierPublicKey
{
public:
    using Ptr = std::shared_ptr<PaillierPublicKey>;
    using UniquePtr = std::unique_ptr<PaillierPublicKey>;
    PaillierPublicKey() = default;
    PaillierPublicKey(bcos::byte const* _pk, unsigned int _len)
    {
        deserialize(_pk, _len);
        precompute();
    }
    virtual ~PaillierPublicKey() = default;

    /////// the stored-elements
    unsigned int keyBits;   // the length of the public key
    ppc::crypto::BigNum n;  // the public key
    ppc::crypto::BigNum h;  // -x^2

    ////// the precomputed-fields
    ppc::crypto::BigNum nSqrt;  // cached for avoid recomputing, precalculation to improve
                                // performance, size is keyBits*2
    ppc::crypto::BigNum h_s;    // h^n mod n^2, precalculation to improve performance, size is
                                // keyBits*2

    // serialize the pk into _encodedData, return the serialized size
    virtual int serialize(bcos::byte* _encodedData, unsigned int _dataLen) const;
    // deserialize the pk
    virtual void deserialize(bcos::byte const* _pk, unsigned int _len);

    void precompute()
    {
        // calculate nSqrt = n^2
        auto ctx = ppc::crypto::createBNContext();
        n.mul(nSqrt.bn().get(), n.bn().get(), ctx.get());
        // calculate h_s = (h^n) mod (n^2)
        h_s = h.modExp(n.bn().get(), nSqrt.bn().get(), ctx.get());
    }

    static unsigned int maxBytes(unsigned int _keyBits)
    {
        // sizeof(keyBits) + keyBits/8(n Bytes) + 2 * keyBits/8(h Bytes) + sizeof(uint16_t) *
        // BigNumFieldNum + BigNumFieldNum
        // = sizeof(keyBits) + (3*KeyBits + 7)/8 + sizeof(uint16_t) * 2  + 2;
        return sizeof(_keyBits) + (3 * _keyBits + 7) / 8 + 6;
    }
};

/// the paillier private key
class PaillierPrivateKey
{
public:
    using Ptr = std::shared_ptr<PaillierPrivateKey>;
    using UniquePtr = std::unique_ptr<PaillierPrivateKey>;
    PaillierPrivateKey() = default;
    PaillierPrivateKey(bcos::byte const* _pk, unsigned int _len)
    {
        deserialize(_pk, _len);
        precompute();
    }
    virtual ~PaillierPrivateKey() = default;

    ////// the stored-fields
    unsigned int keyBits;        // the length of the public key
    ppc::crypto::BigNum lambda;  // the private key, keyBits size
    ppc::crypto::BigNum p;       // p, keyBits/2
    ppc::crypto::BigNum q;       // q, keyBits/2

    ////// the precompute fields
    ppc::crypto::BigNum pSqrt;         // p^2, precalculation to improve performance, keyBits size
    ppc::crypto::BigNum qSqrt;         // q^2, precalculation to improve performance, keyBits size
    ppc::crypto::BigNum qSqrtInverse;  // (q^2)^(-1) mod(p^2), precalculation to improve
                                       // performance, keyBits size
    ppc::crypto::BigNum pOrderSqrt;    // (p*(p-1)), keyBits size
    ppc::crypto::BigNum qOrderSqrt;    // (q*(q - 1)), keyBits size


    // serialize the sk into _encodedData, return the serialized size
    virtual int serialize(bcos::byte* _encodedData, unsigned int _dataLen) const;
    // deserialize the sk
    virtual void deserialize(bcos::byte const* _pk, unsigned int _len);

    virtual void precompute()
    {
        auto ctx = ppc::crypto::createBNContext();
        /// calculate pSqrt and qSqrt
        p.mul(pSqrt.bn().get(), p.bn().get(), ctx.get());
        q.mul(qSqrt.bn().get(), q.bn().get(), ctx.get());
        // (q^2)^(-1) mod(p^2)
        qSqrtInverse = qSqrt.Invert(pSqrt);
        // calculate pOrderSqrt(p*(p-1)) and qOrderSqrt(q*(q - 1))
        pOrderSqrt = pSqrt.sub(p.bn().get());
        qOrderSqrt = qSqrt.sub(q.bn().get());
    }

    static unsigned int maxBytes(unsigned int _keyBits)
    {
        // sizeof(keyBits) + (lambda_length + p_length + q_length) + (lambda_signed_flag +
        // p_signed_flag + q_signed_flag) + (lambda_buffer_len + p_buffer_len + q_buffer_len)
        // = sizeof(keyBits) + (2*_keyBits + 7)/8  + BigNumFieldNum + BigNumFieldNum *
        // = sizeof(keyBits) +  (2*_keyBits + 7)/8 + 3 + 3 * sizeof(int16_t)
        // = sizeof(uint16_t) + (2 * _keyBits + 7)/8 + 9
        return sizeof(_keyBits) + (2 * _keyBits + 7) / 8 + 9;
    }
};

class OpenSSLPaillierKeyPair : public ppc::crypto::KeyPair
{
public:
    using Ptr = std::shared_ptr<OpenSSLPaillierKeyPair>;
    using UniquePtr = std::unique_ptr<OpenSSLPaillierKeyPair>;
    OpenSSLPaillierKeyPair(PaillierPrivateKey::Ptr&& _sk, PaillierPublicKey::Ptr&& _pk)
      : m_sk(std::move(_sk)), m_pk(std::move(_pk))
    {}

    OpenSSLPaillierKeyPair(
        bcos::byte const* _sk, unsigned int _skLen, bcos::byte const* _pk, unsigned int _pkLen)
      : m_sk(std::make_shared<PaillierPrivateKey>(_sk, _skLen)),
        m_pk(std::make_shared<PaillierPublicKey>(_pk, _pkLen))
    {}

    OpenSSLPaillierKeyPair(bcos::bytes const& _sk, bcos::bytes const& _pk)
      : OpenSSLPaillierKeyPair(_sk.data(), _sk.size(), _pk.data(), _pk.size())
    {}

    void* sk() const override { return (void*)m_sk.get(); }
    void* pk() const override { return (void*)m_pk.get(); }

    // serialize the sk
    bcos::bytes serializeSK() const override
    {
        bcos::bytes skData;
        skData.resize(PaillierPrivateKey::maxBytes(m_sk->keyBits));
        if (m_sk)
        {
            auto dataSize = m_sk->serialize(skData.data(), skData.size());
            skData.resize(dataSize);
        }
        return skData;
    }

    // serialize the pk
    bcos::bytes serializePK() const override
    {
        bcos::bytes pkData;
        pkData.resize(PaillierPublicKey::maxBytes(m_pk->keyBits));
        if (m_pk)
        {
            auto dataSize = m_pk->serialize(pkData.data(), pkData.size());
            pkData.resize(dataSize);
        }
        return pkData;
    }

private:
    // the private key
    PaillierPrivateKey::Ptr m_sk;

    // the public key
    PaillierPublicKey::Ptr m_pk;
};
}  // namespace ppc::homo