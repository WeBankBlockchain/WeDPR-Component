/*
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
 * @file CodecUtility.h
 * @author: yujiechen
 * @date 2023-8-11
 */
#pragma once
#include "openssl/bn.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include <bcos-utilities/Common.h>
#include <boost/endian/conversion.hpp>
#include <cstring>
namespace ppc
{
inline bcos::byte* encodeBuffer(
    bcos::byte* _pbuffer, bcos::byte* _pend, bcos::bytesConstRef const& _data)
{
    if (_pend - _pbuffer < (int64_t)sizeof(uint16_t))
    {
        std::out_of_range("Unenough allocated buffer");
    }
    // encode the length of the _data
    uint16_t dataNumBigEndianLen = boost::endian::native_to_big((uint16_t)_data.size());
    std::memcpy((void*)_pbuffer, (const void*)&dataNumBigEndianLen, sizeof(uint16_t));
    _pbuffer += sizeof(uint16_t);
    if (_pend - _pbuffer <= (int64_t)_data.size())
    {
        std::out_of_range("Unenough allocated buffer");
    }
    std::memcpy((void*)_pbuffer, _data.data(), _data.size());
    _pbuffer += _data.size();
    return _pbuffer;
}
/// encode bigNum to the given buffer
// Note: _pbuffer will be updated
inline bcos::byte* encodeBigNum(
    bcos::byte* _pbuffer, bcos::byte* _pend, ppc::crypto::BigNum const& _value)
{
    // convert the BigNum into bytes
    bcos::bytes bigNumData;
    _value.toBytes(bigNumData, true);
    return encodeBuffer(_pbuffer, _pend, bcos::ref(bigNumData));
}

// encode the integer value into buffer
// Note: _pbuffer will be updated
template <typename T>
inline bcos::byte* encodeInteger(bcos::byte* _pbuffer, bcos::byte* _pend, T const _value)
{
    if (_pend - _pbuffer < (int64_t)sizeof(T))
    {
        std::out_of_range("Unenough allocated buffer");
    }
    auto nativeValue = boost::endian::native_to_big(_value);
    std::memcpy((void*)_pbuffer, (const void*)&nativeValue, sizeof(T));
    _pbuffer += sizeof(T);
    return _pbuffer;
}

// decode the bigNum from _buffer[_offset, _offset + sizeof(T)]
inline uint64_t decodeBuffer(bcos::bytes& _result, bcos::byte const* _buffer,
    unsigned int _bufferLen, uint64_t const _offset)
{
    uint64_t offset = _offset;
    CHECK_OFFSET_WITH_THROW_EXCEPTION(offset + 2, _bufferLen);
    // decode the nativeDataLen
    uint16_t nativeDataLen = boost::endian::big_to_native(*((uint16_t*)(_buffer + offset)));
    offset += 2;
    CHECK_OFFSET_WITH_THROW_EXCEPTION(offset + nativeDataLen, _bufferLen);
    // decode the buffer
    _result.insert(_result.begin(), (bcos::byte*)_buffer + offset,
        (bcos::byte*)_buffer + offset + nativeDataLen);
    offset += nativeDataLen;
    return offset;
}

// decode the bigNum from _buffer[_offset, _offset + sizeof(T)]
inline uint64_t decodeBigNum(ppc::crypto::BigNum& _result, bcos::byte const* _buffer,
    unsigned int _bufferLen, uint64_t const _offset)
{
    bcos::bytes decodedBuffer;
    auto offset = decodeBuffer(decodedBuffer, _buffer, _bufferLen, _offset);
    // convert result into BigNum
    _result.fromBytes(bcos::ref(decodedBuffer), true);
    return offset;
}

// decode integer from _buffer + _offset, return the new offset
template <typename T>
inline uint64_t decodeInteger(
    T& _result, bcos::byte const* _buffer, unsigned int _bufferLen, uint64_t _offset)
{
    CHECK_OFFSET_WITH_THROW_EXCEPTION(_offset + sizeof(T), _bufferLen);
    _result = boost::endian::big_to_native(*((T*)(_buffer + _offset)));
    return (_offset + sizeof(T));
}
}  // namespace ppc