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
 * @file PPCMessage.cpp
 * @author: shawnhe
 * @date 2022-10-19
 */

#include "PPCMessage.h"
#include <json/json.h>
#include <boost/asio/detail/socket_ops.hpp>

using namespace bcos;
using namespace ppc::front;

void PPCMessage::encode(bytes& _buffer)
{
    _buffer.clear();

    uint32_t seq = boost::asio::detail::socket_ops::host_to_network_long(m_seq);
    uint16_t taskIDLength = boost::asio::detail::socket_ops::host_to_network_short(m_taskID.size());
    uint16_t senderLength = boost::asio::detail::socket_ops::host_to_network_short(m_sender.size());
    uint32_t dataLength = boost::asio::detail::socket_ops::host_to_network_long(m_data->size());
    uint16_t ext = boost::asio::detail::socket_ops::host_to_network_short(m_ext);

    _buffer.insert(_buffer.end(), (byte*)&m_version, (byte*)&m_version + 1);
    _buffer.insert(_buffer.end(), (byte*)&m_taskType, (byte*)&m_taskType + 1);
    _buffer.insert(_buffer.end(), (byte*)&m_algorithmType, (byte*)&m_algorithmType + 1);
    _buffer.insert(_buffer.end(), (byte*)&m_messageType, (byte*)&m_messageType + 1);
    _buffer.insert(_buffer.end(), (byte*)&seq, (byte*)&seq + 4);
    _buffer.insert(_buffer.end(), (byte*)&taskIDLength, (byte*)&taskIDLength + 2);
    _buffer.insert(_buffer.end(), m_taskID.begin(), m_taskID.end());
    _buffer.insert(_buffer.end(), (byte*)&senderLength, (byte*)&senderLength + 2);
    _buffer.insert(_buffer.end(), m_sender.begin(), m_sender.end());
    _buffer.insert(_buffer.end(), (byte*)&ext, (byte*)&ext + 2);
    // encode the uuid: uuidLen, uuidData
    auto uuidLen = m_uuid.size();
    _buffer.insert(_buffer.end(), (byte*)&uuidLen, (byte*)&uuidLen + 1);
    if (uuidLen > 0)
    {
        _buffer.insert(_buffer.end(), m_uuid.begin(), m_uuid.end());
    }
    // encode the data: dataLen, dataData
    _buffer.insert(_buffer.end(), (byte*)&dataLength, (byte*)&dataLength + 4);
    if (dataLength > 0)
    {
        _buffer.insert(_buffer.end(), m_data->begin(), m_data->end());
    }
    _buffer.insert(_buffer.end(), m_header.begin(), m_header.end());
    m_length = _buffer.size();
}

int64_t PPCMessage::decode(bcos::bytesPointer _buffer)
{
    return decode(_buffer->size(), _buffer->data());
}

int64_t PPCMessage::decode(bytesConstRef _buffer)
{
    return decode(_buffer.size(), (bcos::byte*)_buffer.data());
}

int64_t PPCMessage::decode(uint32_t _length, bcos::byte* _data)
{
    size_t minLen = MESSAGE_MIN_LENGTH;
    if (_length < minLen)
    {
        return -1;
    }

    m_data->clear();
    auto p = _data;

    // version field
    m_version = *((uint8_t*)p);
    p += 1;

    // task type field
    m_taskType = *((uint8_t*)p);
    p += 1;

    // algorithm type field
    m_algorithmType = *((uint8_t*)p);
    p += 1;

    // message type field
    m_messageType = *((uint8_t*)p);
    p += 1;

    // seq field
    m_seq = boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)p));
    p += 4;

    // taskIDLength
    uint16_t taskIDLength = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)p));
    p += 2;
    minLen += taskIDLength;
    if (_length < minLen)
    {
        return -1;
    }

    // taskID field
    m_taskID.insert(m_taskID.begin(), p, p + taskIDLength);
    p += taskIDLength;

    // senderLength
    uint16_t senderLength = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)p));
    p += 2;
    minLen += senderLength;
    if (_length < minLen)
    {
        return -1;
    }
    // sender field
    m_sender.insert(m_sender.begin(), p, p + senderLength);
    p += senderLength;

    // ext field
    m_ext = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)p));
    p += 2;

    // decode the uuid
    auto uuidLen = *((byte*)p);
    p += 1;
    minLen += uuidLen;
    if (_length < minLen)
    {
        return -1;
    }
    if (uuidLen > 0)
    {
        m_uuid.assign(p, p + uuidLen);
        p += uuidLen;
    }

    // dataLength
    uint32_t dataLength = boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)p));
    p += 4;
    minLen += dataLength;
    if (_length < minLen)
    {
        return -1;
    }
    if (dataLength > 0)
    {
        // data field
        m_data->insert(m_data->begin(), p, p + dataLength);
        p += dataLength;
    }

    if (p)
    {
        m_header.insert(m_header.begin(), p, _data + _length);
    }
    m_length = _length;
    return _length;
}

// map<string,string> -> json(string)
std::string PPCMessage::encodeMap(const std::map<std::string, std::string>& _map)
{
    Json::Value pObj;
    for (std::map<std::string, std::string>::const_iterator iter = _map.begin(); iter != _map.end();
         ++iter)
    {
        pObj[iter->first] = iter->second;
    }
    return Json::FastWriter().write(pObj);
}

// json(string) -> map<string,string>
std::map<std::string, std::string> PPCMessage::decodeMap(const std::string& _encval)
{
    Json::Reader reader;
    Json::Value value;
    std::map<std::string, std::string> maps;

    if (_encval.length() > 0)
    {
        if (reader.parse(_encval, value))
        {
            Json::Value::Members members = value.getMemberNames();
            for (Json::Value::Members::iterator it = members.begin(); it != members.end(); it++)
            {
                maps.insert(std::pair<std::string, std::string>(*it, value[*it].asString()));
            }
        }
    }

    return maps;
}