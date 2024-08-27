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

namespace ppc
{
inline uint64_t decodeNetworkBuffer(
    bcos::bytes& _result, bcos::byte const* buffer, unsigned int bufferLen, uint64_t const offset)
{
    uint64_t curOffset = offset;
    CHECK_OFFSET_WITH_THROW_EXCEPTION(curOffset, bufferLen);
    auto dataLen =
        boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)buffer + curOffset));
    curOffset += 2;
    CHECK_OFFSET_WITH_THROW_EXCEPTION(curOffset, bufferLen);
    _result.insert(
        _result.end(), (bcos::byte*)buffer + curOffset, (bcos::byte*)buffer + curOffset + dataLen);
    curOffset += dataLen;
    return curOffset;
}
}  // namespace ppc