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
 * @file PSIMessage.cpp
 * @author: yujiechen
 * @date 2022-11-9
 */
#include "PSIMessage.h"
#include "wedpr-protocol/tars/Common.h"
#include <gperftools/malloc_extension.h>

using namespace ppc::psi;
using namespace bcos;

// destructor the psi message
PSIMessage::~PSIMessage()
{
    m_inner()->data.clear();
    m_inner()->dataIndex.clear();
    std::vector<std::vector<tars::Char>>().swap(m_inner()->data);
    std::vector<long>().swap(m_inner()->dataIndex);
    MallocExtension::instance()->ReleaseFreeMemory();
}
// encode the PSIMessage
bytesPointer PSIMessage::encode() const
{
    auto encodedData = std::make_shared<bytes>();
    tars::TarsOutputStream<ppctars::serialize::BufferWriterByteVector> output;
    m_inner()->writeTo(output);
    output.getByteBuffer().swap(*encodedData);
    return encodedData;
}

// decode the PSIMessage
void PSIMessage::decode(bytesConstRef _data)
{
    tars::TarsInputStream<tars::BufferReader> input;
    input.setBuffer((const char*)_data.data(), _data.size());
    m_inner()->readFrom(input);
}