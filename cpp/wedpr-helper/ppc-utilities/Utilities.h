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
 * @file Utilitiles.cpp
 * @author: yujiechen
 * @date 2024-08-23
 */
#pragma once

#include "ppc-framework/Common.h"
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <random>

namespace ppc
{
template <typename T>
inline uint64_t decodeNetworkBuffer(
    T& _result, bcos::byte const* buffer, unsigned int bufferLen, uint64_t const offset)
{
    uint64_t curOffset = offset;
    CHECK_OFFSET_WITH_THROW_EXCEPTION(curOffset, bufferLen);
    // Notice: operator* is higher priority than operator+, the () is essential
    auto dataLen =
        boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)(buffer + curOffset)));
    curOffset += 2;
    if (dataLen == 0)
    {
        return curOffset;
    }
    CHECK_OFFSET_WITH_THROW_EXCEPTION(curOffset, bufferLen);
    _result.assign((bcos::byte*)buffer + curOffset, (bcos::byte*)buffer + curOffset + dataLen);
    curOffset += dataLen;
    return curOffset;
}

inline std::string generateUUID()
{
    static thread_local auto uuid_gen = boost::uuids::basic_random_generator<std::random_device>();
    return boost::uuids::to_string(uuid_gen());
}
}  // namespace ppc