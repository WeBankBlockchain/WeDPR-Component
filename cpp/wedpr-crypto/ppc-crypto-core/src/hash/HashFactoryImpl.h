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
 * @file HashFactoryImpl.h
 * @author: yujiechen
 * @date 2023-1-3
 */
#pragma once
#include "../Common.h"
#include "BLAKE2bHash.h"
#include "MD5Hash.h"
#include "SM3Hash.h"
#include "Sha256Hash.h"
#include "Sha512Hash.h"
#include "ppc-framework/protocol/Protocol.h"

namespace ppc::crypto
{
class HashFactoryImpl : public HashFactory
{
public:
    using Ptr = std::shared_ptr<HashFactoryImpl>;
    HashFactoryImpl() = default;
    ~HashFactoryImpl() override = default;

    Hash::Ptr createHashImpl(int8_t _hashType) const override
    {
        switch (_hashType)
        {
        case (int8_t)ppc::protocol::HashImplName::SHA256:
            return std::make_shared<Sha256Hash>();
        case (int8_t)ppc::protocol::HashImplName::SHA512:
            return std::make_shared<Sha512Hash>();
        case (int8_t)ppc::protocol::HashImplName::SM3:
            return std::make_shared<SM3Hash>();
        case (int8_t)ppc::protocol::HashImplName::MD5:
            return std::make_shared<MD5Hash>();
        case (int8_t)ppc::protocol::HashImplName::BLAKE2b:
            return std::make_shared<BLAKE2bHash>();
        default:
            BOOST_THROW_EXCEPTION(UnsupportedHashType() << bcos::errinfo_comment(
                                      "unsupported hashType: " + std::to_string(_hashType)));
        }
    }
};
}  // namespace ppc::crypto