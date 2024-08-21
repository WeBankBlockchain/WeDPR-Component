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
 * @file Hash.h
 * @author: yujiechen
 * @date 2022-11-2
 */
#pragma once
#include "../protocol/Protocol.h"
#include <bcos-utilities/Common.h>
#include <memory>
namespace ppc::crypto
{
class HashState
{
public:
    using Ptr = std::shared_ptr<HashState>;
    HashState() = default;
    virtual ~HashState() {}
    virtual void* state() = 0;
};

class Hash
{
public:
    using Ptr = std::shared_ptr<Hash>;
    Hash() = default;
    virtual ~Hash() = default;

    // the hashBytes length of given hash-algorithm
    virtual size_t hashLen() const = 0;
    // the implementation of the hash-algorithm
    virtual ppc::protocol::HashImplName type() const = 0;

    // calculate-hash
    virtual bcos::bytes hash(bcos::bytesConstRef _input) const = 0;
    // init the hash-state
    virtual HashState::Ptr init() const = 0;
    // update new message into the given hash-state
    virtual void update(HashState::Ptr _state, bcos::bytesConstRef _data) const = 0;
    // obtain the hash-result from the given hash-state
    virtual bcos::bytes final(HashState::Ptr _state) const = 0;
};

class HashFactory
{
public:
    using Ptr = std::shared_ptr<HashFactory>;
    HashFactory() = default;
    virtual ~HashFactory() = default;

    virtual Hash::Ptr createHashImpl(int8_t _hashType) const = 0;
};
}  // namespace ppc::crypto