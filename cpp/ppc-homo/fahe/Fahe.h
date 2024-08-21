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
 * @file Fahe.h
 * @author: yujiechen
 * @date 2023-08-30
 */
#pragma once
#include "openssl/bn.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include <bcos-utilities/Common.h>
#include <stdint.h>
#include <memory>
namespace ppc::homo
{
DERIVE_PPC_EXCEPTION(FaheException);
struct FaheKey
{
    int16_t lambda;
    int16_t alpha;
    int16_t pos;
    int16_t mmax;
    ppc::crypto::BigNum p;
    ppc::crypto::BigNum x;

    using UniquePtr = std::unique_ptr<FaheKey>;
    FaheKey() = default;
};
// FAHE2 implementation:
//  Fast Additive Partially Homomorphic Encryption from the Approximate Common Divisor problem
class Fahe
{
public:
    using Ptr = std::shared_ptr<Fahe>;
    Fahe() = default;
    virtual ~Fahe() = default;

    virtual FaheKey::UniquePtr generateKey(int16_t _lambda, int16_t _mmax, int16_t _alpha);
    virtual ppc::crypto::BigNum encrypt(BIGNUM const* _plain, FaheKey const* _key);
    virtual ppc::crypto::BigNum decrypt(BIGNUM const* _cipher, FaheKey const* _key);
    virtual ppc::crypto::BigNum add(BIGNUM const* _cipher1, BIGNUM const* _cipher2);
};
}  // namespace ppc::homo