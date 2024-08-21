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
 * @file KeyPair.h
 * @author: yujiechen
 * @date 2023-08-04
 */
#pragma once
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::crypto
{
class KeyPair
{
public:
    using Ptr = std::shared_ptr<KeyPair>;
    using UniquePtr = std::unique_ptr<KeyPair>;
    KeyPair() = default;
    virtual ~KeyPair() = default;

    virtual void* sk() const = 0;
    virtual void* pk() const = 0;

    // serialize the sk
    virtual bcos::bytes serializeSK() const = 0;
    // serialize the pk
    virtual bcos::bytes serializePK() const = 0;
};
}  // namespace ppc::crypto