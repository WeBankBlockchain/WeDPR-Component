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
 * @file Ihc.h
 * @author: asherli
 * @date 2023-11-23
 */

#pragma once
#include "../libwrapper/BigNum.h"
#include "../libwrapper/Buffer.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::homo
{
class Ihc
{
public:
    enum class IhcMode
    {
        IHC_128,
        IHC_256,
    };

    using Ptr = std::shared_ptr<Ihc>;
    Ihc() = default;
    virtual ~Ihc() = default;

    // generate key according to given mode
    virtual bcos::bytes generateKey() const = 0;
    virtual void generateKey(OutputBuffer* _result) const = 0;

    // encrypt the given value to cipher using given key
    virtual void encrypt(
        OutputBuffer* _cipher, bcos::bytesConstRef const& _key, BIGNUM const* _value) const = 0;
    virtual bcos::bytes encrypt(bcos::bytesConstRef const& _key, BIGNUM const* _value) const = 0;
    // decrypt the given cipher to value using given key
    virtual ppc::crypto::BigNum decrypt(
        bcos::bytesConstRef const& _key, bcos::bytesConstRef const& _cipher) const = 0;
    // ihc add
    virtual void add(OutputBuffer* _addResult, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2) const = 0;
    virtual bcos::bytes add(
        bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const = 0;
    // ihc sub
    virtual void sub(OutputBuffer* _subResult, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2) const = 0;
    virtual bcos::bytes sub(
        bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const = 0;

    // ihc scalaMul
    virtual void scalaMul(OutputBuffer* _mulResult, BIGNUM const* _value,
        bcos::bytesConstRef const& _cipher) const = 0;
    virtual bcos::bytes scalaMul(
        BIGNUM const* _value, bcos::bytesConstRef const& _cipher) const = 0;

    virtual unsigned int keyBits() const = 0;
    virtual unsigned int keyBytes() const = 0;
    virtual uint64_t cipherBytes() const = 0;
};
}  // namespace ppc::homo