/**
 *  Copyright (C) 2021 FISCO BCOS.
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
 * @author: yujiechen
 * @date 2021-04-12
 */
#pragma once
#include "bcos-utilities/Common.h"
#include "ppc-framework/Common.h"

namespace ppc::protocol
{
DERIVE_PPC_EXCEPTION(PBObjectEncodeException);
DERIVE_PPC_EXCEPTION(PBObjectDecodeException);

template <typename T>
void encodePBObject(bcos::bytes& _encodedData, T _pbObject)
{
    auto encodedData = std::make_shared<bcos::bytes>();
    _encodedData.resize(_pbObject->ByteSizeLong());
    if (!_pbObject->SerializeToArray(_encodedData.data(), _encodedData.size()))
    {
        BOOST_THROW_EXCEPTION(PBObjectEncodeException()
                              << bcos::errinfo_comment("encode PBObject into bytes data failed"));
    }
}

template <typename T>
void decodePBObject(T _pbObject, bcos::bytesConstRef _data)
{
    if (!_pbObject->ParseFromArray(_data.data(), _data.size()))
    {
        BOOST_THROW_EXCEPTION(PBObjectDecodeException()
                              << bcos::errinfo_comment("decode bytes data into PBObject failed"));
    }
}
}  // namespace ppc::protocol
