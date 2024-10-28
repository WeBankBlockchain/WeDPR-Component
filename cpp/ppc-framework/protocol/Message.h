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
 * @file Message.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#pragma once
#include "MessagePayload.h"
#include "RouteType.h"
#include "ppc-framework/Helper.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <memory>
#include <sstream>
namespace ppc::protocol
{
class MessageOptionalHeader
{
public:
    using Ptr = std::shared_ptr<MessageOptionalHeader>;
    MessageOptionalHeader() = default;
    virtual ~MessageOptionalHeader() = default;

    virtual void encode(bcos::bytes& buffer) const = 0;
    virtual int64_t decode(bcos::bytesConstRef data, uint64_t const _offset) = 0;

    // the componentType
    virtual std::string componentType() const { return m_componentType; }
    virtual void setComponentType(std::string componentType) { m_componentType = componentType; }

    // the source nodeID that send the message
    virtual bcos::bytes const& srcNode() const { return m_srcNode; }
    /// for swig-wrapper(pass the binary data)
    OutputBuffer srcNodeBuffer() const
    {
        // Note: this will be copied to java through jni
        return OutputBuffer{(unsigned char*)m_srcNode.data(), m_srcNode.size()};
    }

    virtual void setSrcNode(bcos::bytes const& srcNode) { m_srcNode = srcNode; }

    // !!! Note: the first paramater type should not been changed, for it's used for pass-in java
    // byte[] into c bytes
    virtual void setSrcNodeBuffer(char* data, uint64_t length)
    {
        m_srcNode.assign(data, data + length);
    }

    // the target nodeID that should receive the message
    virtual bcos::bytes const& dstNode() const { return m_dstNode; }

    // for swig-wrapper(pass the binary to java)
    OutputBuffer dstNodeBuffer() const
    {
        // Note: this will be copied to java through jni
        return OutputBuffer{(unsigned char*)m_dstNode.data(), m_dstNode.size()};
    }
    virtual void setDstNode(bcos::bytes const& dstNode)
    {
        m_dstNode = dstNode;
        m_dstNodePtr = bcos::bytesConstRef((bcos::byte*)m_dstNode.data(), m_dstNode.size());
    }
    virtual void setDstNodePtr(bcos::bytesConstRef const& dstNodePtr) { m_dstNodePtr = dstNodePtr; }
    // !!! Note: the first paramater type(char*) should not been changed, for it's used for pass-in
    // java byte[] into c bytes
    // Note: the python not support function override
    virtual void setDstNodeBuffer(char* data, uint64_t length)
    {
        m_dstNode.assign(data, data + length);
    }

    // the target agency that need receive the message
    virtual std::string const& dstInst() const { return m_dstInst; }
    virtual void setDstInst(std::string const& dstInst) { m_dstInst = dstInst; }

    // the topic
    virtual std::string const& topic() const { return m_topic; }
    virtual void setTopic(std::string&& topic) { m_topic = std::move(topic); }
    virtual void setTopic(std::string const& topic) { m_topic = topic; }

    virtual std::string srcInst() const { return m_srcInst; }
    virtual void setSrcInst(std::string const& srcInst) { m_srcInst = srcInst; }

protected:
    std::string m_topic;
    // the componentType
    std::string m_componentType;
    // the source nodeID that send the message
    bcos::bytes m_srcNode;
    // the source agency
    std::string m_srcInst;
    // the target nodeID that should receive the message
    bcos::bytes m_dstNode;
    bcos::bytesConstRef m_dstNodePtr;  // to decrease the copy overhead
    // the target agency that need receive the message
    std::string m_dstInst;
};

class MessageHeader
{
public:
    using Ptr = std::shared_ptr<MessageHeader>;
    MessageHeader() = default;
    virtual ~MessageHeader() = default;

    virtual void encode(bcos::bytes& buffer) const = 0;
    virtual int64_t decode(bcos::bytesConstRef data) = 0;

    // the msg version, used to support compatibility
    virtual uint8_t version() const { return m_version; }
    virtual void setVersion(uint16_t version) { m_version = version; }
    // the traceID
    virtual std::string const& traceID() const { return m_traceID; }
    virtual void setTraceID(std::string traceID) { m_traceID = traceID; }

    // the srcGwNode
    virtual std::string const& srcGwNode() const { return m_srcGwNode; }
    virtual void setSrcGwNode(std::string const& srcGwNode) { m_srcGwNode = srcGwNode; }

    // the dstGwNode
    virtual std::string const& dstGwNode() const { return m_dstGwNode; }
    virtual void setDstGwNode(std::string const& dstGwNode) { m_dstGwNode = dstGwNode; }

    // the packetType
    virtual uint16_t packetType() const { return m_packetType; }
    virtual void setPacketType(uint16_t packetType) { m_packetType = packetType; }
    // the ttl
    virtual int16_t ttl() const { return m_ttl; }
    virtual void setTTL(uint16_t ttl) { m_ttl = ttl; }

    // the ext(contains the router policy and response flag)
    virtual uint16_t ext() const { return m_ext; }
    virtual void setExt(uint16_t ext) { m_ext = ext; }
    //// the optional field(used to route between components and nodes)
    virtual MessageOptionalHeader::Ptr optionalField() const { return m_optionalField; }
    void setOptionalField(MessageOptionalHeader::Ptr optionalField)
    {
        m_optionalField = std::move(optionalField);
    }

    virtual uint16_t length() const { return m_length; }

    virtual bool isRespPacket() const = 0;
    virtual void setRespPacket() = 0;


    // Note: only for log
    std::string_view srcP2PNodeIDView() const { return printP2PIDElegantly(m_srcGwNode); }
    // Note: only for log
    std::string_view dstP2PNodeIDView() const { return printP2PIDElegantly(m_dstGwNode); }

    virtual uint16_t routeType() const = 0;
    virtual void setRouteType(ppc::protocol::RouteType type) = 0;
    virtual bool hasOptionalField() const = 0;

protected:
    // Note: must init here to 0, otherwise, it will be unexpected value in some other platform
    // the msg version, used to support compatibility
    uint8_t m_version = 0;
    // the traceID
    std::string m_traceID;
    // the srcGwNode
    std::string m_srcGwNode;
    // the dstGwNode
    std::string m_dstGwNode;
    // the packetType
    uint16_t m_packetType = 0;
    // the ttl
    int16_t m_ttl = 0;
    // the ext(contains the router policy and response flag)
    uint16_t m_ext = 0;
    //// the optional field(used to route between components and nodes)
    MessageOptionalHeader::Ptr m_optionalField;
    uint16_t mutable m_length;
};

class Message
{
public:
    using Ptr = std::shared_ptr<Message>;
    Message() = default;
    virtual ~Message() {}

    virtual MessageHeader::Ptr header() const { return m_header; }
    virtual void setHeader(MessageHeader::Ptr header) { m_header = std::move(header); }
    /// the overloaed implementation ===
    uint16_t version() const { return m_header->version(); }
    void setVersion(uint16_t version) { m_header->setVersion(version); }
    uint16_t packetType() const { return m_header->packetType(); }
    void setPacketType(uint16_t packetType) { m_header->setPacketType(packetType); }
    std::string const& seq() const { return m_header->traceID(); }
    void setSeq(std::string traceID) { m_header->setTraceID(traceID); }
    uint16_t ext() const { return m_header->ext(); }
    void setExt(uint16_t ext) { m_header->setExt(ext); }

    bool isRespPacket() const { return m_header->isRespPacket(); }
    void setRespPacket() { m_header->setRespPacket(); }
    void setPayload(std::shared_ptr<bcos::bytes> _payload) { m_payload = std::move(_payload); }
    // for swig wrapper
    OutputBuffer payloadBuffer() const
    {
        if (!m_payload)
        {
            return OutputBuffer{nullptr, 0};
        }
        return OutputBuffer{(unsigned char*)m_payload->data(), m_payload->size()};
    }

    void setFrontMessage(MessagePayload::Ptr frontMessage)
    {
        m_frontMessage = std::move(frontMessage);
    }

    MessagePayload::Ptr const& frontMessage() const { return m_frontMessage; }

    // Note: swig wrapper require define all methods
    virtual bool encode(bcos::bytes& _buffer) = 0;
    virtual int64_t decode(bcos::bytesConstRef _buffer) = 0;

    virtual uint32_t length() const
    {
        return m_header->length() + (m_payload ? m_payload->size() : 0);
    }

    virtual std::shared_ptr<bcos::bytes> payload() const { return m_payload; }

protected:
    MessageHeader::Ptr m_header;
    // Note: allocate here in case of wsService nullptr access caused coredump
    std::shared_ptr<bcos::bytes> m_payload = std::make_shared<bcos::bytes>();
    MessagePayload::Ptr m_frontMessage = nullptr;
};

class MessageHeaderBuilder
{
public:
    using Ptr = std::shared_ptr<MessageHeaderBuilder>;
    MessageHeaderBuilder() = default;
    virtual ~MessageHeaderBuilder() = default;

    virtual MessageHeader::Ptr build(bcos::bytesConstRef _data) = 0;
    virtual MessageHeader::Ptr build() = 0;
    virtual MessageOptionalHeader::Ptr build(MessageOptionalHeader::Ptr const& optionalHeader) = 0;
};

class MessageBuilder
{
public:
    using Ptr = std::shared_ptr<MessageBuilder>;
    MessageBuilder() = default;
    virtual ~MessageBuilder() = default;

    virtual Message::Ptr build() = 0;
    virtual Message::Ptr build(bcos::bytesConstRef buffer) = 0;
    virtual Message::Ptr build(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload) = 0;
};

class MessageOptionalHeaderBuilder
{
public:
    using Ptr = std::shared_ptr<MessageOptionalHeaderBuilder>;
    MessageOptionalHeaderBuilder() = default;
    virtual ~MessageOptionalHeaderBuilder() = default;

    virtual MessageOptionalHeader::Ptr build(MessageOptionalHeader::Ptr const& optionalHeader) = 0;
    virtual MessageOptionalHeader::Ptr build() = 0;
};

inline std::string printOptionalField(MessageOptionalHeader::Ptr optionalHeader)
{
    if (!optionalHeader)
    {
        return "nullptr";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("topic", optionalHeader->topic())
                 << LOG_KV("componentType", optionalHeader->componentType())
                 << LOG_KV("srcNode", printNodeID(optionalHeader->srcNode()))
                 << LOG_KV("dstNode", printNodeID(optionalHeader->dstNode()))
                 << LOG_KV("srcInst", printNodeID(optionalHeader->srcInst()))
                 << LOG_KV("dstInst", printNodeID(optionalHeader->dstInst()));
    return stringstream.str();
}

inline std::string printMessage(Message::Ptr const& _msg)
{
    if (!_msg)
    {
        return "nullptr";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("from", _msg->header()->srcP2PNodeIDView())
                 << LOG_KV("to", _msg->header()->dstP2PNodeIDView())
                 << LOG_KV("routeType", (ppc::protocol::RouteType)_msg->header()->routeType())
                 << LOG_KV("ttl", _msg->header()->ttl())
                 << LOG_KV("rsp", _msg->header()->isRespPacket())
                 << LOG_KV("traceID", _msg->header()->traceID())
                 << LOG_KV("packetType", _msg->header()->packetType())
                 << LOG_KV("length", _msg->length());
    if (_msg->header()->hasOptionalField())
    {
        stringstream << printOptionalField(_msg->header()->optionalField());
    }
    return stringstream.str();
}
// function to send response
using SendResponseFunction = std::function<void(std::shared_ptr<bcos::bytes>&& payload)>;
using ReceiveMsgFunc = std::function<void(bcos::Error::Ptr)>;
using MessageCallback = std::function<void(
    bcos::Error::Ptr e, ppc::protocol::Message::Ptr msg, SendResponseFunction resFunc)>;
using MessageDispatcherCallback = std::function<void(ppc::protocol::Message::Ptr)>;
}  // namespace ppc::protocol