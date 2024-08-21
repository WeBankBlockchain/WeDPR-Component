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
 * @file Ore.h
 * @author: shawnhe
 * @date 2023-08-18
 */
#pragma once

#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::crypto
{
class Ore
{
public:
    using Ptr = std::shared_ptr<Ore>;
    Ore() = default;
    virtual ~Ore() = default;

    virtual bcos::bytes generateKey() const = 0;
    virtual void generateKey(OutputBuffer* sk) const = 0;
    virtual int keyBytes() const = 0;

    // cipher encoded with hex
    virtual std::string encrypt4String(
        bcos::bytesConstRef const& _sk, const std::string& _plaintext) const = 0;
    virtual void encrypt4String(OutputBuffer* _cipher, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _plaintext) const = 0;

    // cipher encoded with hex
    virtual std::string decrypt4String(
        bcos::bytesConstRef const& _sk, const std::string& _ciphertext) const = 0;
    virtual void decrypt4String(OutputBuffer* plain, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& cipher) const = 0;

    // cipher encoded with base64
    virtual std::string encrypt4Integer(
        bcos::bytesConstRef const& _sk, const int64_t& _plain) const = 0;
    virtual void encrypt4Integer(
        OutputBuffer* _cipher, bcos::bytesConstRef const& _sk, const int64_t& _plain) const = 0;

    // cipher encoded with base64
    virtual int64_t decrypt4Integer(
        bcos::bytesConstRef const& _sk, const std::string& _cipher) const = 0;
    virtual void decrypt4Integer(int64_t* _plain, bcos::bytesConstRef const& _sk,
        bcos::bytesConstRef const& _cipher) const = 0;

    // cipher encoded with base64
    virtual std::string encrypt4Float(
        bcos::bytesConstRef const& _sk, const float50& _plain) const = 0;
    virtual void encrypt4Float(
        OutputBuffer* _cipher, bcos::bytesConstRef const& _sk, const float50& _plain) const = 0;

    // cipher encoded with base64
    virtual float50 decrypt4Float(
        bcos::bytesConstRef const& _sk, const std::string& _cipher) const = 0;
    virtual float50 decrypt4Float(
        bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& _cipher) const = 0;

    virtual int compare(const std::string& _ciphertext0, const std::string& _ciphertext1) const = 0;
    virtual int compare(InputBuffer const* c1, InputBuffer const* c2) const = 0;
};
}  // namespace ppc::crypto