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
 * @file FloatingPointPaillier.cpp
 * @author: yujiechen
 * @date 2023-08-17
 */

#include "FloatingPointPaillier.h"
#include "../codec/Common.h"

using namespace ppc::homo;
using namespace ppc::crypto;
using namespace ppc;

void FloatingPointPaillier::encryptFast(
    OutputBuffer* _result, FloatingPointNumber const& _value, void* _keyPair) const
{
    auto cipher = m_paillierImpl->encrypt_with_crt(_value.value.bn().get(), _keyPair);
    FloatingPointCipher cipherObject(std::move(cipher), _value.exponent);
    cipherObject.encode(_result);
}

void FloatingPointPaillier::encrypt(
    OutputBuffer* _result, FloatingPointNumber const& _value, void* _publicKey) const
{
    auto cipher = m_paillierImpl->encrypt(_value.value.bn().get(), _publicKey);
    FloatingPointCipher cipherObject(std::move(cipher), _value.exponent);
    cipherObject.encode(_result);
}


FloatingPointNumber FloatingPointPaillier::decrypt(
    bcos::bytesConstRef const& _cipherData, void* _keyPair) const
{
    FloatingPointCipher cipherObject(_cipherData);
    // decrypt
    auto decodedV = m_paillierImpl->decrypt(bcos::ref(cipherObject.cipher()), _keyPair);
    return FloatingPointNumber(std::move(decodedV), cipherObject.exponent());
}

void FloatingPointPaillier::align(
    FloatingPointCipher& _c1, FloatingPointCipher& _c2, void* _publicKey) const
{
    if (!_publicKey)
    {
        BOOST_THROW_EXCEPTION(FloatingPointPaillierException() << bcos::errinfo_comment(
                                  "precisionAlign for two-cipher error for invalid public key"));
    }
    auto self = weak_from_this();
    precisionAlign(_c1, _c2,
        [self, _publicKey](BIGNUM const* v, bcos::bytesConstRef const& _cipher) -> bcos::bytes {
            auto fpPaillier = self.lock();
            if (!fpPaillier)
            {
                return bcos::bytes();
            }
            return fpPaillier->m_paillierImpl->scalaMul(v, _cipher, _publicKey);
        });
}

void FloatingPointPaillier::add(OutputBuffer* _result, bcos::bytesConstRef const& _c1,
    bcos::bytesConstRef const& _c2, void* _publicKey) const
{
    FloatingPointCipher c1Obj(_c1);
    FloatingPointCipher c2Obj(_c2);
    // align the precision
    align(c1Obj, c2Obj, _publicKey);
    auto addResult =
        m_paillierImpl->add(bcos::ref(c1Obj.cipher()), bcos::ref(c2Obj.cipher()), _publicKey);
    FloatingPointCipher cipherAddResult(std::move(addResult), c1Obj.exponent());
    cipherAddResult.encode(_result);
}

void FloatingPointPaillier::sub(OutputBuffer* _result, bcos::bytesConstRef const& _c1,
    bcos::bytesConstRef const& _c2, void* _publicKey) const
{
    FloatingPointCipher c1Obj(_c1);
    FloatingPointCipher c2Obj(_c2);
    // align the precision
    align(c1Obj, c2Obj, _publicKey);
    auto subResult =
        m_paillierImpl->sub(bcos::ref(c1Obj.cipher()), bcos::ref(c2Obj.cipher()), _publicKey);
    FloatingPointCipher cipherSubResult(std::move(subResult), c1Obj.exponent());
    cipherSubResult.encode(_result);
}

void FloatingPointPaillier::scalaMul(OutputBuffer* _result, FloatingPointNumber const& _v,
    bcos::bytesConstRef const& _c, void* _publicKey) const
{
    if (!_publicKey)
    {
        BOOST_THROW_EXCEPTION(FloatingPointPaillierException()
                              << bcos::errinfo_comment("scalaMul error for invalid public key"));
    }
    FloatingPointCipher cipherObject(_c);
    auto mulResult =
        m_paillierImpl->scalaMul(_v.value.bn().get(), bcos::ref(cipherObject.cipher()), _publicKey);
    FloatingPointCipher cipherMulResult(
        std::move(mulResult), cipherObject.exponent() + _v.exponent);
    cipherMulResult.encode(_result);
}