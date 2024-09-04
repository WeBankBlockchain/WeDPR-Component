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
 * @file MmapParser.h
 * @author: yujiechen
 * @date 2022-10-20
 */
#pragma once
#include "../Common.h"
#include "BaseBufferParser.h"
#include <sys/mman.h>
#include <memory>

namespace ppc::io
{
class MmapParser : public BaseBufferParser
{
public:
    using Ptr = std::unique_ptr<MmapParser>;
    MmapParser(int _fd, uint64_t _offset, uint64_t _length, bool _readOnly = true)
      : BaseBufferParser()
    {
        m_length = _length;
        int prot = PROT_READ;
        if (!_readOnly)
        {
            prot = PROT_READ | PROT_WRITE;
        }
        // Note: the mmap offset param must be a multiple of page-size(4096), otherwise, Invalid
        // Argument error will be raised
        m_address = (char*)mmap(nullptr, _length, prot, MAP_SHARED, _fd, _offset);
        if (MAP_FAILED == m_address)
        {
            BOOST_THROW_EXCEPTION(MmapFileException() << bcos::errinfo_comment(
                                      "mmap file exception, code: " + std::to_string(errno) +
                                      ", error: " + strerror(errno)));
        }
        IO_LOG(INFO) << LOG_DESC("MmapFile success") << LOG_KV("offset", _offset)
                     << LOG_KV("length", _length) << LOG_KV("readOnly", _readOnly)
                     << ", address:" << std::hex << (uint64_t)m_address;
    }

    ~MmapParser() override
    {
        if (!m_address)
        {
            return;
        }
        munmap(m_address, m_length);
    }
};
}  // namespace ppc::io