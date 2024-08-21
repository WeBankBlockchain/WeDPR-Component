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
 * @file BufferParser.h
 * @author: yujiechen
 * @date 2022-11-30
 */
#pragma once
#include "BaseBufferParser.h"

namespace ppc::io
{
class BufferParser : public BaseBufferParser
{
public:
    using Ptr = std::shared_ptr<BufferParser>;
    BufferParser(std::vector<bcos::byte>&& _data) : BaseBufferParser(), m_data(std::move(_data))
    {
        m_length = m_data.size();
        m_address = (char*)m_data.data();
    }
    ~BufferParser() override = default;

protected:
    std::vector<bcos::byte> m_data;
};
}  // namespace ppc::io