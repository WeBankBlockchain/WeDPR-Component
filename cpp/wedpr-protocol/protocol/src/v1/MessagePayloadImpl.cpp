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
 * @file MessagePayloadImpl.h
 * @author: yujiechen
 * @date 2024-08-22
 */

#include "MessagePayloadImpl.h"

#include "ppc-utilities/Utilities.h"
#include <boost/asio/detail/socket_ops.hpp>

using namespace ppc::protocol;
using namespace bcos;

int64_t MessagePayloadImpl::encode(bcos::bytes& buffer) const
{
    // version
    uint16_t version = boost::asio::detail::socket_ops::host_to_network_short(m_version);
    buffer.insert(buffer.end(), (byte*)&version, (byte*)&version + 2);
    // seq
    uint16_t seq = boost::asio::detail::socket_ops::host_to_network_short(m_seq);
    buffer.insert(buffer.end(), (byte*)&seq, (byte*)&seq + 2);
    // ext field
    uint16_t ext = boost::asio::detail::socket_ops::host_to_network_short(m_ext);
    buffer.insert(buffer.end(), (byte*)&ext, (byte*)&ext + 2);
    // traceID
    uint16_t traceIDLen = boost::asio::detail::socket_ops::host_to_network_short(m_traceID.size());
    buffer.insert(buffer.end(), (byte*)&traceIDLen, (byte*)&traceIDLen + 2);
    buffer.insert(buffer.end(), m_traceID.begin(), m_traceID.end());
    // data
    uint32_t dataLen = boost::asio::detail::socket_ops::host_to_network_long(m_dataPtr.size());
    buffer.insert(buffer.end(), (byte*)&dataLen, (byte*)&dataLen + 4);
    buffer.insert(buffer.end(), m_dataPtr.begin(), m_dataPtr.end());
    // update the length
    m_length = buffer.size();
    return m_length;
}

int64_t MessagePayloadImpl::decode(bcos::bytesConstRef buffer)
{
    // check the message
    if (buffer.size() < MIN_PAYLOAD_LEN)
    {
        BOOST_THROW_EXCEPTION(
            WeDPRException() << errinfo_comment("Malform payload for too small!"));
    }
    m_length = buffer.size();
    auto pointer = buffer.data();
    // the version
    m_version = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the seq
    CHECK_OFFSET_WITH_THROW_EXCEPTION((pointer - buffer.data()), buffer.size());
    m_seq = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the ext
    CHECK_OFFSET_WITH_THROW_EXCEPTION((pointer - buffer.data()), buffer.size());
    m_ext = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the traceID
    auto offset =
        decodeNetworkBuffer(m_traceID, buffer.data(), buffer.size(), (pointer - buffer.data()));
    // data
    auto ret = decodeNetworkBuffer(m_data, buffer.data(), buffer.size(), offset, true);
    // reset the dataPtr
    m_dataPtr = bcos::bytesConstRef((bcos::byte*)m_data.data(), m_data.size());
    return ret;
}