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
 * @file MessageImpl.cpp
 * @author: yujiechen
 * @date 2024-08-23
 */

#include "MessageImpl.h"

using namespace bcos;
using namespace ppc::protocol;

bool MessageImpl::encode(bcos::bytes& _buffer)
{
    // encode the header
    bcos::bytes headerData;
    m_header->encode(headerData);
    // encode the payload
    if (m_payload)
    {
        headerData.insert(headerData.end(), m_payload->begin(), m_payload->end());
    }
}

bool MessageImpl::encode(bcos::boostssl::EncodedMsg& encodedMsg)
{
    // header
    m_header->encode(encodedMsg.header);
    // assign the payload back
    encodedMsg.payload = m_payload;
}

int64_t MessageImpl::decode(bytesConstRef buffer)
{
    if (buffer.size() > m_maxMessageLen)
    {
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "Malform message for over the size limit, max allowed size is: " +
                                  std::to_string(m_maxMessageLen)));
    }
    // decode the header
    m_header = m_headerBuilder->build(buffer);
    // decode the payload
    if (!m_payload)
    {
        m_payload = std::make_shared<bcos::bytes>();
    }
    m_payload->clear();
    m_payload->insert(m_payload->end(), buffer.data() + m_header->length(), buffer.end());
}