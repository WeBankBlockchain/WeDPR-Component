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
 * @file Paillier.h
 * @author: yujiechen
 * @date 2023-08-04
 */
#pragma once
#include "../libwrapper/BigNum.h"
#include "../libwrapper/Buffer.h"
#include "KeyPair.h"

namespace ppc::homo
{
class Paillier
{
public:
    using Ptr = std::shared_ptr<Paillier>;
    Paillier() = default;
    virtual ~Paillier() = default;

    // generate the keypair
    virtual ppc::crypto::KeyPair::UniquePtr generateKeyPair(unsigned const _keyLength) const = 0;
    // encrypt the _value
    virtual bcos::bytes encrypt_with_crt(BIGNUM const* _value, void* _keyPair) const = 0;
    // encrypt with crt optimization
    virtual void encrypt_with_crt(OutputBuffer* _result, OutputBuffer* _rBuffer,
        BIGNUM const* _value, void* _keyPair) const = 0;
    // encrypt without crt optimization
    virtual void encrypt(
        OutputBuffer* _result, OutputBuffer* _rBuffer, BIGNUM const* _value, void* _pk) const = 0;
    virtual bcos::bytes encrypt(BIGNUM const* _value, void* _pk) const = 0;

    // decrypt the cipher data
    virtual ppc::crypto::BigNum decrypt(
        bcos::bytesConstRef const& _cipherData, void* _keyPair) const = 0;

    // _cipher1 add _cipher2
    virtual bcos::bytes add(bcos::bytesConstRef const& _cipher1,
        bcos::bytesConstRef const& _cipher2, void* _publicKey) const = 0;
    virtual void add(OutputBuffer* _result, bcos::bytesConstRef const& _cipher1,
        bcos::bytesConstRef const& _cipher2, void* _publicKey) const = 0;

    // _cipher1 sub _cipher2
    virtual bcos::bytes sub(bcos::bytesConstRef const& _cipher1,
        bcos::bytesConstRef const& _cipher2, void* _publicKey) const = 0;
    virtual void sub(OutputBuffer* _result, bcos::bytesConstRef const& _cipher1,
        bcos::bytesConstRef const& _cipher2, void* _publicKey) const = 0;

    // v * _cipher
    virtual bcos::bytes scalaMul(
        BIGNUM const* v, bcos::bytesConstRef const& _cipher, void* _publicKey) const = 0;
    virtual void scalaMul(OutputBuffer* _result, BIGNUM const* v,
        bcos::bytesConstRef const& _cipher, void* _publicKey) const = 0;
};
}  // namespace ppc::homo