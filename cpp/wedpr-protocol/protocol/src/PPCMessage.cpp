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
#include "Common.h"
#include <json/json.h>
#include <boost/asio/detail/socket_ops.hpp>

using namespace bcos;
using namespace ppc::front;
using namespace ppc::protocol;

void PPCMessage::encode(bytes& _buffer)
{
    _buffer.clear();

    uint32_t dataLength = boost::asio::detail::socket_ops::host_to_network_long(m_data->size());

    _buffer.insert(_buffer.end(), (byte*)&m_version, (byte*)&m_version + 1);
    _buffer.insert(_buffer.end(), (byte*)&m_taskType, (byte*)&m_taskType + 1);
    _buffer.insert(_buffer.end(), (byte*)&m_algorithmType, (byte*)&m_algorithmType + 1);
    _buffer.insert(_buffer.end(), (byte*)&m_messageType, (byte*)&m_messageType + 1);
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

    if (p < _data + _length)
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

// Note: this interface is used after the MessagePayload(frontMessage) has been decoded; this
// interface passed some meta information to the ppcMessage
PPCMessageFace::Ptr PPCMessageFactory::decodePPCMessage(Message::Ptr msg)
{
    auto ppcMsg = buildPPCMessage();
    auto frontMsg = msg->frontMessage();
    // Note: this field is been setted when onReceiveMessage
    if (frontMsg)
    {
        ppcMsg->decode(bcos::ref(frontMsg->data()));
        ppcMsg->setSeq(frontMsg->seq());
        ppcMsg->setUuid(frontMsg->traceID());
        if (frontMsg->isRespPacket())
        {
            ppcMsg->setResponse();
        }
    }
    if (msg->header() && msg->header()->optionalField())
    {
        auto const& routeInfo = msg->header()->optionalField();
        ppcMsg->setTaskID(routeInfo->topic());
        ppcMsg->setSender(routeInfo->srcInst());
        ppcMsg->setSenderNode(routeInfo->srcNode());
    }
    return ppcMsg;
}

Message::Ptr PPCMessageFactory::buildMessage(MessageBuilder::Ptr const& msgBuilder,
    MessagePayloadBuilder::Ptr const& msgPayloadBuilder, PPCMessageFace::Ptr const& ppcMessage)
{
    auto msg = msgBuilder->build();
    msg->header()->optionalField()->setTopic(ppcMessage->taskID());
    msg->header()->optionalField()->setSrcInst(ppcMessage->sender());

    auto payload = buildMessage(msgPayloadBuilder, ppcMessage);
    auto payloadData = std::make_shared<bcos::bytes>();
    payload->encode(*payloadData);
    msg->setPayload(std::move(payloadData));
    return msg;
}

MessagePayload::Ptr PPCMessageFactory::buildMessage(
    MessagePayloadBuilder::Ptr const& msgPayloadBuilder, PPCMessageFace::Ptr const& ppcMessage)
{
    auto payload = msgPayloadBuilder->build();
    payload->setSeq(ppcMessage->seq());
    if (ppcMessage->response())
    {
        payload->setRespPacket();
    }
    payload->setTraceID(ppcMessage->uuid());

    bcos::bytes ppcMsgData;
    ppcMessage->encode(ppcMsgData);
    payload->setData(std::move(ppcMsgData));
    return payload;
}