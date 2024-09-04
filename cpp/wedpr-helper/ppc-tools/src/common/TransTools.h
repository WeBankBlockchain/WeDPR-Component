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
 * @file TransTools.h
 * @author: shawnhe
 * @date 2022-11-07
 */

#pragma once

#include <bcos-utilities/Common.h>
#include <boost/asio/detail/socket_ops.hpp>

namespace ppc::tools
{
template <typename UnsignedT>
inline void encodeUnsignedNum(bcos::bytesPointer _buffer, UnsignedT _number)
{
    UnsignedT temp;
    if (sizeof(_number) == 2)
    {
        temp = boost::asio::detail::socket_ops::host_to_network_short(_number);
    }
    else if (sizeof(_number) == 4)
    {
        temp = boost::asio::detail::socket_ops::host_to_network_long(_number);
    }
    else
    {
        temp = _number;
    }

    _buffer->clear();
    _buffer->assign((bcos::byte*)&temp, (bcos::byte*)&temp + sizeof(_number));
}

template <typename UnsignedT>
inline void encodeUnsignedNum(bcos::byte* _buffer, UnsignedT _number)
{
    UnsignedT temp;
    if (sizeof(_number) == 2)
    {
        temp = boost::asio::detail::socket_ops::host_to_network_short(_number);
    }
    else if (sizeof(_number) == 4)
    {
        temp = boost::asio::detail::socket_ops::host_to_network_long(_number);
    }
    else
    {
        temp = _number;
    }

    memcpy(_buffer, (bcos::byte*)&temp, sizeof(_number));
}

template <typename UnsignedT>
inline void encodeUnsignedNum(std::string& _res, UnsignedT _number)
{
    _res = "";
    UnsignedT temp;
    auto _buffer = std::make_shared<bcos::bytes>();
    if (sizeof(_number) == 2)
    {
        temp = boost::asio::detail::socket_ops::host_to_network_short(_number);
    }
    else if (sizeof(_number) == 4)
    {
        temp = boost::asio::detail::socket_ops::host_to_network_long(_number);
    }
    else
    {
        temp = _number;
    }

    _buffer->clear();
    _buffer->assign((bcos::byte*)&temp, (bcos::byte*)&temp + sizeof(_number));
    _res.assign(_buffer->begin(), _buffer->end());
}

template <typename UnsignedT>
inline void decodeUnsignedNum(UnsignedT& _number, const std::string& _restr)
{
    auto _buffer = std::make_shared<bcos::bytes>(bcos::bytes(_restr.begin(), _restr.end()));
    if (sizeof(_number) == 1)
    {
        _number = *((uint8_t*)_buffer->data());
    }
    else if (sizeof(_number) == 2)
    {
        _number =
            boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)_buffer->data()));
    }
    else if (sizeof(_number) == 4)
    {
        _number =
            boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)_buffer->data()));
    }
    else
    {
        _number = UnsignedT(*((UnsignedT*)_buffer->data()));
    }
}

template <typename UnsignedT>
inline void decodeUnsignedNum(UnsignedT& _number, bcos::bytesPointer _buffer)
{
    if (sizeof(_number) == 1)
    {
        _number = *((uint8_t*)_buffer->data());
    }
    else if (sizeof(_number) == 2)
    {
        _number =
            boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)_buffer->data()));
    }
    else if (sizeof(_number) == 4)
    {
        _number =
            boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)_buffer->data()));
    }
    else
    {
        _number = UnsignedT(*((UnsignedT*)_buffer->data()));
    }
}

template <typename UnsignedT>
inline void decodeUnsignedNum(UnsignedT& _number, bcos::byte* _buffer)
{
    if (sizeof(_number) == 1)
    {
        _number = *((uint8_t*)_buffer);
    }
    else if (sizeof(_number) == 2)
    {
        _number = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)_buffer));
    }
    else if (sizeof(_number) == 4)
    {
        _number = boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)_buffer));
    }
    else
    {
        _number = UnsignedT(*((UnsignedT*)_buffer));
    }
}

template <typename T>
T toBigEndian(T value)
{
    T result = 0;
    uint8_t* resultBytes = reinterpret_cast<uint8_t*>(&result);
    uint8_t* valueBytes = reinterpret_cast<uint8_t*>(&value);

    for (size_t i = 0; i < sizeof(T); ++i)
    {
        resultBytes[i] = valueBytes[sizeof(T) - 1 - i];
    }

    return result;
}

template <typename T>
T fromBigEndian(T value)
{
    T result = 0;
    uint8_t* resultBytes = reinterpret_cast<uint8_t*>(&result);
    uint8_t* valueBytes = reinterpret_cast<uint8_t*>(&value);

    for (size_t i = 0; i < sizeof(T); ++i)
    {
        resultBytes[i] = valueBytes[sizeof(T) - 1 - i];
    }

    return result;
}

}  // namespace ppc::tools
