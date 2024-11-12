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
#include "Message.h"
#include <bcos-boostssl/interfaces/MessageFace.h>
namespace ppc::protocol
{
// the wrapper for override bcos::boostssl::MessageFace while use implementation for Message
class P2PMessage : virtual public bcos::boostssl::MessageFace
{
public:
    using Ptr = std::shared_ptr<P2PMessage>;
    P2PMessage(Message::Ptr msg) : m_msg(std::move(msg)) {}
    ~P2PMessage() override {}

    /// the overloaed implementation ===
    uint16_t version() const override { return m_msg->version(); }
    void setVersion(uint16_t version) override { m_msg->setVersion(version); }
    uint16_t packetType() const override { return m_msg->packetType(); }
    void setPacketType(uint16_t packetType) override { m_msg->setPacketType(packetType); }
    std::string const& seq() const override { return m_msg->seq(); }
    void setSeq(std::string seq) override { m_msg->setSeq(seq); }
    uint16_t ext() const override { return m_msg->ext(); }
    void setExt(uint16_t ext) override { m_msg->setExt(ext); }
    std::shared_ptr<bcos::bytes> payload() const override { return m_msg->payload(); }
    void setPayload(std::shared_ptr<bcos::bytes> payload) override
    {
        m_msg->setPayload(std::move(payload));
    }

    bool encode(bcos::bytes& _buffer) override { return m_msg->encode(_buffer); }

    int64_t decode(bcos::bytesConstRef _buffer) override { return m_msg->decode(_buffer); }

    bool isRespPacket() const override { return m_msg->isRespPacket(); }
    void setRespPacket() override { m_msg->setRespPacket(); }
    uint32_t length() const override { return m_msg->length(); }

    // encode and return the {header, payload}
    virtual bool encode(bcos::boostssl::EncodedMsg& _encodedMsg) = 0;

    Message::Ptr const& msg() { return m_msg; }
    MessageHeader::Ptr header() const { return m_msg->header(); }

protected:
    Message::Ptr m_msg;
};


class P2PMessageBuilder : virtual public bcos::boostssl::MessageFaceFactory
{
public:
    using Ptr = std::shared_ptr<P2PMessageBuilder>;
    P2PMessageBuilder() = default;
    ~P2PMessageBuilder() override = default;

    virtual P2PMessage::Ptr build() = 0;
    virtual P2PMessage::Ptr build(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload) = 0;
};
inline std::string printWsMessage(bcos::boostssl::MessageFace::Ptr const& _msg)
{
    if (!_msg)
    {
        return "nullptr";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("rsp", _msg->isRespPacket()) << LOG_KV("traceID", _msg->seq())
                 << LOG_KV("packetType", _msg->packetType()) << LOG_KV("length", _msg->length())
                 << LOG_KV("ext", _msg->ext());
    return stringstream.str();
}
inline std::string printP2PMessage(P2PMessage::Ptr const& _msg)
{
    if (_msg->msg() == nullptr)
    {
        return "";
    }
    return printMessage(_msg->msg());
}
}  // namespace ppc::protocol
