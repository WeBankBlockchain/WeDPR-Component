


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
 * @file Common.h
 * @author: asherli
 * @date 2023-12-29
 */
#pragma once
#include "FloatingPointCipher.h"
#include "openssl/bn.h"
namespace ppc::homo
{
inline void precisionAlign(FloatingPointCipher& _c1, FloatingPointCipher& _c2,
    std::function<bcos::bytes(BIGNUM const* v, bcos::bytesConstRef const& _cipher)> const&
        _alignFunc)
{
    if (_c1.exponent() == _c2.exponent())
    {
        return;
    }
    auto alignedCipher = &_c1;
    int exp = 0;
    // c1.exp < c2.exp, align exponent of c2 to the exponent of c1
    if (_c1.exponent() < _c2.exponent())
    {
        alignedCipher = &_c2;
        exp = _c2.exponent() - _c1.exponent();
        alignedCipher->setExponent(_c1.exponent());
    }
    else
    {
        // c1 > c2, align exponent of c1 to the exponent of c2
        exp = _c1.exponent() - _c2.exponent();
        alignedCipher->setExponent(_c2.exponent());
    }
    ppc::crypto::BigNum expBN(exp);
    auto ctx = ppc::crypto::createBNContext();
    ppc::crypto::BigNum c_base = ppc::crypto::BigNum(10);
    auto v = c_base.exp(expBN.bn().get(), ctx.get());
    auto cipher = _alignFunc(v.bn().get(), bcos::ref(alignedCipher->cipher()));
    alignedCipher->setCipher(std::move(cipher));
}
}  // namespace ppc::homo