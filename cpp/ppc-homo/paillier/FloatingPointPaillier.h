/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file FloatingPointPaillier.h
 * @author: yujiechen
 * @date 2023-08-17
 */
#pragma once
#include "../codec/FloatingPointCipher.h"
#include "../codec/SignedNumberCodec.h"
#include "OpenSSLPaillier.h"
#include "ppc-framework/crypto/Paillier.h"
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include <memory>

namespace ppc::homo
{
class FloatingPointPaillier : public std::enable_shared_from_this<FloatingPointPaillier>
{
public:
    using Ptr = std::shared_ptr<FloatingPointPaillier>;
    FloatingPointPaillier(Paillier::Ptr const& _paillierImpl) : m_paillierImpl(_paillierImpl) {}
    virtual ~FloatingPointPaillier() = default;

    virtual void encryptFast(
        OutputBuffer* _result, ppc::FloatingPointNumber const& _value, void* _keyPair) const;
    virtual void encrypt(
        OutputBuffer* _result, ppc::FloatingPointNumber const& _value, void* _publicKey) const;
    virtual ppc::FloatingPointNumber decrypt(
        bcos::bytesConstRef const& _cipherData, void* _keyPair) const;

    virtual void add(OutputBuffer* _result, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& c2, void* _publicKey) const;
    virtual void sub(OutputBuffer* _result, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& c2, void* _publicKey) const;

    // Note: the FloatingPointNumber maybe modified when aligning
    virtual void scalaMul(OutputBuffer* _result, ppc::FloatingPointNumber const& _v,
        bcos::bytesConstRef const& c, void* _publicKey) const;

    Paillier::Ptr const paillierImpl() const { return m_paillierImpl; }
    static unsigned int maxCipherBytesLen(unsigned int _keyBits)
    {
        return sizeof(uint16_t) + sizeof(int16_t) + OpenSSLPaillier::maxCipherBytesLen(_keyBits);
    }

private:
    // align the exponent between cipher c1 and c2
    void align(FloatingPointCipher& c1, FloatingPointCipher& c2, void* _publicKey) const;

private:
    Paillier::Ptr m_paillierImpl;
    const ppc::crypto::BigNum C_BASE = ppc::crypto::BigNum(10);
};
}  // namespace ppc::homo