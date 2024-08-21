/**
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License") {}
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
 * @file FloatingPointIhc.h
 * @author: asherli
 * @date 2023-12-29
 */
#include "FloatingPointIhc.h"

using namespace ppc::homo;
using namespace ppc;

void FloatingPointIhc::encrypt(
    OutputBuffer* _result, bcos::bytesConstRef const& sk, FloatingPointNumber const& _value) const
{
    auto cipher = m_ihc->encrypt(sk, _value.value.bn().get());
    FloatingPointCipher cipherObject(std::move(cipher), _value.exponent);
    cipherObject.encode(_result);
}

FloatingPointNumber FloatingPointIhc::decrypt(
    bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& _cipher) const
{
    FloatingPointCipher cipherObject(_cipher);
    auto resultV = m_ihc->decrypt(_sk, bcos::ref(cipherObject.cipher()));
    return FloatingPointNumber(std::move(resultV), cipherObject.exponent());
}

void FloatingPointIhc::add(
    OutputBuffer* _result, bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const
{
    FloatingPointCipher cipher1(_c1);
    FloatingPointCipher cipher2(_c2);
    align(cipher1, cipher2);
    auto cipher = m_ihc->add(bcos::ref(cipher1.cipher()), bcos::ref(cipher2.cipher()));
    FloatingPointCipher result(std::move(cipher), cipher1.exponent());
    result.encode(_result);
}

void FloatingPointIhc::sub(
    OutputBuffer* _result, bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const
{
    FloatingPointCipher cipher1(_c1);
    FloatingPointCipher cipher2(_c2);
    align(cipher1, cipher2);
    auto cipher = m_ihc->sub(bcos::ref(cipher1.cipher()), bcos::ref(cipher2.cipher()));
    FloatingPointCipher result(std::move(cipher), cipher1.exponent());
    result.encode(_result);
}

void FloatingPointIhc::scalaMul(
    OutputBuffer* _result, FloatingPointNumber const& _v, bcos::bytesConstRef const& _c) const
{
    FloatingPointCipher cipher(_c);
    auto resultBytes = m_ihc->scalaMul(_v.value.bn().get(), bcos::ref(cipher.cipher()));
    auto exponent = _v.exponent + cipher.exponent();
    FloatingPointCipher result(std::move(resultBytes), exponent);
    result.encode(_result);
}