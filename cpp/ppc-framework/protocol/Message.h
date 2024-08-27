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
#include "../Common.h"
#include "RouteType.h"
#include <bcos-boostssl/interfaces/MessageFace.h>
#include <bcos-utilities/Common.h>
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
    virtual void setSrcNode(bcos::bytes const& srcNode) { m_srcNode = srcNode; }

    // the target nodeID that should receive the message
    virtual bcos::bytes const& dstNode() const { return m_dstNode; }
    virtual void setDstNode(bcos::bytes const& dstNode) { m_dstNode = dstNode; }

    // the target agency that need receive the message
    virtual std::string const& dstInst() const { return m_dstInst; }
    virtual void setDstInst(std::string const& dstInst) { m_dstInst = dstInst; }

    // the topic
    virtual std::string const& topic() const { return m_topic; }
    virtual void setTopic(std::string&& topic) { m_topic = std::move(topic); }
    virtual void setTopic(std::string const& topic) { m_topic = topic; }

protected:
    std::string m_topic;
    // the componentType
    std::string m_componentType;
    // the source nodeID that send the message
    bcos::bytes m_srcNode;
    // the target nodeID that should receive the message
    bcos::bytes m_dstNode;
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

protected:
    // the msg version, used to support compatibility
    uint8_t m_version = 0;
    // the traceID
    std::string m_traceID;
    // the srcGwNode
    std::string m_srcGwNode;
    // the dstGwNode
    std::string m_dstGwNode;
    // the packetType
    uint16_t m_packetType;
    // the ttl
    int16_t m_ttl = 0;
    // the ext(contains the router policy and response flag)
    uint16_t m_ext;
    //// the optional field(used to route between components and nodes)
    MessageOptionalHeader::Ptr m_optionalField;
    uint16_t mutable m_length;
};

class Message : virtual public bcos::boostssl::MessageFace
{
public:
    using Ptr = std::shared_ptr<Message>;
    Message() = default;
    ~Message() override {}

    virtual MessageHeader::Ptr header() const { return m_header; }
    virtual void setHeader(MessageHeader::Ptr header) { m_header = std::move(header); }

    /// the overloaed implementation ===
    uint16_t version() const override { return m_header->version(); }
    void setVersion(uint16_t version) override { m_header->setVersion(version); }
    uint16_t packetType() const override { return m_header->packetType(); }
    void setPacketType(uint16_t packetType) override { m_header->setPacketType(packetType); }
    std::string const& seq() const override { return m_header->traceID(); }
    void setSeq(std::string traceID) override { m_header->setTraceID(traceID); }
    uint16_t ext() const override { return m_header->ext(); }
    void setExt(uint16_t ext) override { m_header->setExt(ext); }

    bool isRespPacket() const override { return m_header->isRespPacket(); }
    void setRespPacket() override { m_header->setRespPacket(); }

    virtual uint32_t length() const override
    {
        return m_header->length() + (m_payload ? m_payload->size() : 0);
    }

    std::shared_ptr<bcos::bytes> payload() const override { return m_payload; }
    void setPayload(std::shared_ptr<bcos::bytes> _payload) override
    {
        m_payload = std::move(_payload);
    }

protected:
    MessageHeader::Ptr m_header;
    std::shared_ptr<bcos::bytes> m_payload;
};

class MessageHeaderBuilder
{
public:
    using Ptr = std::shared_ptr<MessageHeaderBuilder>;
    MessageHeaderBuilder() = default;
    virtual ~MessageHeaderBuilder() = default;

    virtual MessageHeader::Ptr build(bcos::bytesConstRef _data) = 0;
    virtual MessageHeader::Ptr build() = 0;
};

class MessageBuilder : public bcos::boostssl::MessageFaceFactory
{
public:
    using Ptr = std::shared_ptr<MessageBuilder>;
    MessageBuilder() = default;
    ~MessageBuilder() override = default;

    virtual Message::Ptr build() = 0;
    virtual Message::Ptr build(bcos::bytesConstRef buffer) = 0;
    virtual Message::Ptr build(ppc::protocol::RouteType routeType, std::string const& topic,
        std::string const& dstInst, bcos::bytes const& dstNodeID, std::string const& componentType,
        bcos::bytes&& payload) = 0;
};

inline std::string printMessage(Message::Ptr const& _msg)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("from", _msg->header()->srcP2PNodeIDView())
                 << LOG_KV("to", _msg->header()->dstP2PNodeIDView())
                 << LOG_KV("ttl", _msg->header()->ttl())
                 << LOG_KV("rsp", _msg->header()->isRespPacket())
                 << LOG_KV("traceID", _msg->header()->traceID())
                 << LOG_KV("packetType", _msg->header()->packetType())
                 << LOG_KV("length", _msg->length());
    return stringstream.str();
}

inline std::string printWsMessage(bcos::boostssl::MessageFace::Ptr const& _msg)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("rsp", _msg->isRespPacket()) << LOG_KV("traceID", _msg->seq())
                 << LOG_KV("packetType", _msg->packetType()) << LOG_KV("length", _msg->length());
    return stringstream.str();
}

// function to send response
using SendResponseFunction = std::function<void(ppc::protocol::Message::Ptr msg)>;
using ReceiveMsgFunc = std::function<void(bcos::Error::Ptr)>;
using MessageCallback = std::function<void(
    bcos::Error::Ptr e, ppc::protocol::Message::Ptr msg, SendResponseFunction resFunc)>;

}  // namespace ppc::protocol