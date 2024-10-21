/*
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
 * @file Common.h
 * @author: yujiechen
 * @date 2022-10-20
 */
#pragma once
#include <sstream>
#include <string>
namespace ppc
{
constexpr static int MAX_PORT = 65535;
constexpr static int DEFAULT_SECURITY_PARAM = 128;

constexpr static size_t RSA_PUBLIC_KEY_PREFIX = 18;
constexpr static size_t RSA_PUBLIC_KEY_TRUNC = 8;
constexpr static size_t RSA_PUBLIC_KEY_TRUNC_LENGTH = 26;

inline std::string_view printP2PIDElegantly(std::string_view p2pId) noexcept
{
    if (p2pId.length() < RSA_PUBLIC_KEY_TRUNC_LENGTH)
    {
        return p2pId;
    }
    return p2pId.substr(RSA_PUBLIC_KEY_PREFIX, RSA_PUBLIC_KEY_TRUNC);
}


template <typename T>
inline std::string_view printNodeID(T const& nodeID)
{
    return std::string_view((const char*)nodeID.data(), nodeID.size());
}

template <typename T>
inline std::string printCollection(T const& collection)
{
    std::ostringstream oss;
    for (auto const& it : collection)
    {
        oss << it << ",";
    }
    return oss.str();
}
}  // namespace ppc