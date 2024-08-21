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
 * @file BaseBufferParser.h
 * @author: yujiechen
 * @date 2022-11-30
 */
#pragma once
#include <memory>
namespace ppc::io
{
enum class ReadLineResult
{
    ReadFinish,
    ReadFinishWithoutSpliter,
    NotReadFinish,
};
class BaseBufferParser
{
public:
    using Ptr = std::shared_ptr<BaseBufferParser>;
    BaseBufferParser() = default;
    BaseBufferParser(char* _address, uint64_t _length) : m_address(_address), m_length(_length) {}
    virtual ~BaseBufferParser() = default;

    virtual void* address() const { return (void*)m_address; }
    virtual uint64_t length() const { return m_length; }

    template <typename T>
    ReadLineResult readLine(int64_t& readBytes, T& _lineData, char _delim = '\n')
    {
        if (m_pointer >= m_length)
        {
            return ReadLineResult::ReadFinish;
        }
        auto orgPointer = m_pointer;
        auto startAddr = m_address + m_pointer;
        uint64_t copyLen = 0;
        bool findDelim = false;
        for (; m_pointer < m_length; m_pointer++)
        {
            while (m_pointer < m_length && (((char)*(m_address + m_pointer) == _delim) ||
                                               ((char)*(m_address + m_pointer) == c_ignoreDelim)))
            {
                // skip the delim when read-line next-round
                m_pointer++;
                findDelim = true;
            }
            if (findDelim)
            {
                break;
            }
            copyLen++;
        }
        readBytes = (uint64_t)(m_pointer - orgPointer);
        _lineData.resize(copyLen);
        memcpy(_lineData.data(), (char*)startAddr, copyLen);
        // not read-finished
        if (m_pointer < m_length)
        {
            return ReadLineResult::NotReadFinish;
        }
        if (findDelim)
        {
            return ReadLineResult::ReadFinish;
        }
        return ReadLineResult::ReadFinishWithoutSpliter;
    }

    virtual void readAll(bcos::bytes& _data)
    {
        _data.insert(_data.end(), (char*)m_address, (char*)m_address + m_length);
        m_pointer = m_length;
    }
    virtual uint64_t pointer() const { return m_pointer; }

protected:
    char* m_address = nullptr;
    uint64_t m_length;
    uint64_t m_pointer = 0;

    const char c_ignoreDelim = '\r';
};
}  // namespace ppc::io