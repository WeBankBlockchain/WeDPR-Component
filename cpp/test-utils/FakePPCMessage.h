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
 * @brief fake ppc network message interface
 * @file FakePPCMessage.h
 * @author: yujiechen
 * @date 2022-11-16
 */
#pragma once
#include "ppc-framework/protocol/PPCMessageFace.h"

using namespace ppc::front;
namespace ppc::test
{
class FakePPCMessage : public PPCMessageFace
{
public:
    using Ptr = std::shared_ptr<FakePPCMessage>;
    FakePPCMessage() = default;
    ~FakePPCMessage() override = default;

    uint8_t version() const override { return m_version; }
    void setVersion(uint8_t _version) override { m_version = _version; }
    uint8_t taskType() const override { return m_taskType; }
    void setTaskType(uint8_t _taskType) override { m_taskType = _taskType; }
    uint8_t algorithmType() const override { return m_algorithmType; }
    void setAlgorithmType(uint8_t _algorithmType) override { m_algorithmType = _algorithmType; }
    uint8_t messageType() const override { return m_messageType; }
    void setMessageType(uint8_t _messageType) override { m_messageType = _messageType; }
    uint32_t seq() const override { return m_seq; }
    void setSeq(uint32_t _seq) override { m_seq = _seq; }
    std::string const& taskID() const override { return m_taskID; }
    void setTaskID(std::string const& _taskID) override { m_taskID = _taskID; }
    std::map<std::string, std::string> header() override { return m_header; }
    void setHeader(std::map<std::string, std::string> _header) override { m_header = _header; }

    std::string const& sender() const override { return m_sender; }
    void setSender(std::string const& _sender) override { m_sender = _sender; }

    std::shared_ptr<bcos::bytes> data() const override { return m_data; }
    void setData(std::shared_ptr<bcos::bytes> _data) override { m_data = _data; }
    // Note: we don't fake the encode-decode here
    void encode(bcos::bytes&) override
    {
        throw std::runtime_error("FakePPCMessage: unimplemented interface!");
    }
    int64_t decode(bcos::bytesPointer) override
    {
        throw std::runtime_error("FakePPCMessage: unimplemented interface!");
    }
    int64_t decode(bcos::bytesConstRef) override
    {
        throw std::runtime_error("FakePPCMessage: unimplemented interface!");
    }
    uint32_t length() const override { return 0; }

    std::string uuid() const override { return m_uuid; }
    void setUuid(std::string const& _uuid) override { m_uuid = _uuid; }

    // determine the message is response or not
    bool response() const override { return m_response; }
    // set the message to be response
    void setResponse() override { m_response = true; }


    bcos::bytes const& senderNode() const override { return m_senderNode; }

    void setSenderNode(bcos::bytes const& senderNode) override { m_senderNode = senderNode; }

private:
    bcos::bytes m_senderNode;
    uint8_t m_version;
    uint8_t m_taskType;
    uint8_t m_algorithmType;
    uint8_t m_messageType;
    uint32_t m_seq;
    std::string m_taskID;
    std::string m_sender;
    uint16_t m_ext;
    std::shared_ptr<bcos::bytes> m_data;
    std::string m_uuid;
    std::map<std::string, std::string> m_header;
    bool m_response = false;
};

class FakePPCMessageFactory : public PPCMessageFaceFactory
{
public:
    using Ptr = std::shared_ptr<FakePPCMessageFactory>;
    FakePPCMessageFactory() = default;
    ~FakePPCMessageFactory() override = default;
    PPCMessageFace::Ptr buildPPCMessage() override { return std::make_shared<FakePPCMessage>(); }
    PPCMessageFace::Ptr buildPPCMessage(bcos::bytesConstRef) override
    {
        throw std::runtime_error("FakePPCMessageFactory: unimplemented interface!");
    }

    PPCMessageFace::Ptr buildPPCMessage(bcos::bytesPointer) override
    {
        throw std::runtime_error("FakePPCMessageFactory: unimplemented interface!");
    }

    PPCMessageFace::Ptr buildPPCMessage(uint8_t _taskType, uint8_t _algorithmType,
        std::string const& _taskID, bcos::bytesPointer _data) override
    {
        throw std::runtime_error("FakePPCMessageFactory: unimplemented interface!");
    }
    PPCMessageFace::Ptr decodePPCMessage(ppc::protocol::Message::Ptr msg) override
    {
        throw std::runtime_error("FakePPCMessageFactory: unimplemented interface!");
    }
    ppc::protocol::Message::Ptr buildMessage(ppc::protocol::MessageBuilder::Ptr const& msgBuilder,
        ppc::protocol::MessagePayloadBuilder::Ptr const& msgPayloadBuilder,
        PPCMessageFace::Ptr const& ppcMessage) override
    {
        throw std::runtime_error("FakePPCMessageFactory: unimplemented interface!");
    }

    ppc::protocol::MessagePayload::Ptr buildMessage(
        ppc::protocol::MessagePayloadBuilder::Ptr const& msgPayloadBuilder,
        PPCMessageFace::Ptr const& ppcMessage) override
    {
        throw std::runtime_error("FakePPCMessageFactory: unimplemented interface!");
    }
};
}  // namespace ppc::test