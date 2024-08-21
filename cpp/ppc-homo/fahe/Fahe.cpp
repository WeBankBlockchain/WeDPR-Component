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
 * @file Fahe.cpp
 * @author: yujiechen
 * @date 2023-08-30
 */
// clang-format off
#include <math.h>
#include <random>
// clang-format on
#include "Fahe.h"
#include "../codec/SignedNumberCodec.h"
using namespace ppc::homo;
using namespace ppc::crypto;
using namespace std;

FaheKey::UniquePtr Fahe::generateKey(int16_t _lambda, int16_t _mmax, int16_t _alpha)
{
    auto key = std::make_unique<FaheKey>();
    key->lambda = _lambda;
    key->alpha = _alpha;
    key->mmax = _mmax;
    srand(bcos::utcTime());

    key->pos = rand() % _lambda;
    double tmp = _lambda + _alpha + _mmax;
    auto n = tmp + _alpha;
    // p
    key->p.generatePrime(n);
    // x = 2^r/p
    BigNum base(2);
    // r = p/lgp *（(n-tmp)^2）
    // divider = (log10(_lambda + _alpha + _mmax));
    int16_t r = (int16_t)((double)tmp / (log10((double)tmp)) * _alpha * _alpha);
    BigNum exponent(r);
    auto ctx = createBNContext();
    key->x = base.exp(exponent.bn().get(), ctx.get());
    key->x.div(key->x.bn().get(), NULL, key->p.bn().get(), ctx.get());
    return key;
}

BigNum Fahe::encrypt(BIGNUM const* _m, FaheKey const* _key)
{
    // convert _m to signed
    SignedNumberCodec codec(_key->mmax);
    auto m = codec.encode(_m);

    auto noise1 = generateRand(_key->pos);
    auto noise2 = generateRand(_key->lambda - _key->pos);
    // M = (noise2 << (pos + mmax + alpha) + (m << (pos + alpha)) + noise1)
    BigNum M1(noise2);
    auto shiftStep = _key->pos + _key->mmax + _key->alpha;
    if (BN_lshift(M1.bn().get(), M1.bn().get(), shiftStep) != 1)
    {
        BOOST_THROW_EXCEPTION(
            FaheException() << bcos::errinfo_comment(
                "encrypt error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
    }
    BigNum M2;
    if (BN_lshift(M2.bn().get(), m.bn().get(), (_key->pos + _key->alpha)) != 1)
    {
        BOOST_THROW_EXCEPTION(
            FaheException() << bcos::errinfo_comment(
                "encrypt error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
    }
    auto M = M1.add(M2.bn().get()).add(noise1.bn().get());
    // select q from [0, x)
    BigNum q;
    randRange(q.bn().get(), _key->x.bn().get());
    // n = p * q
    BigNum n;
    auto ctx = createBNContext();
    q.mul(n.bn().get(), _key->p.bn().get(), ctx.get());
    // c = n + M
    return n.add(M.bn().get());
}

BigNum Fahe::decrypt(BIGNUM const* _cipher, FaheKey const* _key)
{
    BigNum result;
    /// result = (c mod p) >> (pos + alpha)
    // cipher mod p
    auto ctx = createBNContext();
    if (BN_div(NULL, result.bn().get(), _cipher, _key->p.bn().get(), ctx.get()) != 1)
    {
        BOOST_THROW_EXCEPTION(
            FaheException() << bcos::errinfo_comment(
                "decrypt error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
    }
    // result = (cipher mod p) >> (pos + alpha)
    if (BN_rshift(result.bn().get(), result.bn().get(), (_key->pos + _key->alpha)) != 1)
    {
        BOOST_THROW_EXCEPTION(
            FaheException() << bcos::errinfo_comment(
                "decrypt error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
    }
    BN_mask_bits(result.bn().get(), _key->mmax);
    SignedNumberCodec codec(_key->mmax);
    return codec.decode(result);
}

BigNum Fahe::add(BIGNUM const* _cipher1, BIGNUM const* _cipher2)
{
    BigNum result;
    if (BN_add(result.bn().get(), _cipher1, _cipher2) != 1)
    {
        BOOST_THROW_EXCEPTION(
            FaheException() << bcos::errinfo_comment(
                "add error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
    }
    return result;
}
