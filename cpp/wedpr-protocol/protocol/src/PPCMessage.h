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
 * @brief ppc network message interface
 * @file PPCMessage.h
 * @author: shawnhe
 * @date 2022-10-19
 */

#pragma once

#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "ppc-framework/protocol/PPCMessageFace.h"

namespace ppc
{
namespace front
{
// the message format for ppc protocol
class PPCMessage : public PPCMessageFace
{
public:
    // version(1) + taskType(1) + algorithmType(1) + messageType(1)
    // + dataLen(4) + data(N)
    // + header(M)
    const static size_t MESSAGE_MIN_LENGTH = 8;

    using Ptr = std::shared_ptr<PPCMessage>;
    PPCMessage() { m_data = std::make_shared<bcos::bytes>(); }
    // Note: the payload passed in by the upper layer cannot be released at will
    ~PPCMessage() override = default;

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
    std::string const& sender() const override { return m_sender; }
    void setSender(std::string const& _sender) override { m_sender = _sender; }
    std::shared_ptr<bcos::bytes> data() const override { return m_data; }
    // Note: here directly use passed-in _data, make-sure _data not changed before send the message
    void setData(std::shared_ptr<bcos::bytes> _data) override { m_data = _data; }
    std::map<std::string, std::string> header() override { return decodeMap(m_header); }
    void setHeader(std::map<std::string, std::string> _header) override
    {
        m_header = encodeMap(_header);
    }

    std::string uuid() const override { return m_uuid; }
    void setUuid(std::string const& _uuid) override { m_uuid = _uuid; }

    void encode(bcos::bytes& _buffer) override;
    int64_t decode(bcos::bytesPointer _buffer) override;
    int64_t decode(bcos::bytesConstRef _buffer) override;
    int64_t decode(uint32_t _length, bcos::byte* _data);

    uint32_t length() const override { return m_length; }

    // determine the message is response or not
    bool response() const override { return m_isResponse; }
    // set the message to be response
    void setResponse() override { m_isResponse = true; }

    void setSenderNode(bcos::bytes const& senderNode) override { m_senderNode = senderNode; }

    bcos::bytes const& senderNode() const override { return m_senderNode; }

    void releasePayload() override
    {
        if (!m_data)
        {
            return;
        }
        m_data->clear();
        bcos::bytes().swap(*m_data);
    }

protected:
    std::string encodeMap(const std::map<std::string, std::string>& _map);
    std::map<std::string, std::string> decodeMap(const std::string& _encval);

private:
    uint8_t m_version = 0;
    uint8_t m_taskType = 0;
    uint8_t m_algorithmType = 0;
    uint8_t m_messageType = 0;
    uint32_t m_seq = 0;
    std::string m_taskID;
    std::string m_sender;
    bcos::bytes m_senderNode;

    bool m_isResponse;
    // the uuid used to find the response-callback
    std::string m_uuid;
    std::shared_ptr<bcos::bytes> m_data;
    std::string m_header;

    uint32_t m_length = MESSAGE_MIN_LENGTH;
};

class PPCMessageFactory : public PPCMessageFaceFactory
{
public:
    using Ptr = std::shared_ptr<PPCMessageFactory>;
    PPCMessageFactory() = default;
    virtual ~PPCMessageFactory() {}

public:
    PPCMessageFace::Ptr buildPPCMessage() override { return std::make_shared<PPCMessage>(); }

    PPCMessageFace::Ptr buildPPCMessage(uint8_t _taskType, uint8_t _algorithmType,
        std::string const& _taskID, std::shared_ptr<bcos::bytes> _data) override
    {
        auto msg = std::make_shared<PPCMessage>();
        msg->setTaskType(_taskType);
        msg->setAlgorithmType(_algorithmType);
        msg->setTaskID(_taskID);
        msg->setData(std::move(_data));
        return msg;
    }


    PPCMessageFace::Ptr buildPPCMessage(bcos::bytesConstRef _buffer) override
    {
        auto msg = std::make_shared<PPCMessage>();
        int64_t length = msg->decode(_buffer);
        if (length == -1)
        {
            return nullptr;
        }
        return msg;
    }

    PPCMessageFace::Ptr buildPPCMessage(bcos::bytesPointer _buffer) override
    {
        auto msg = std::make_shared<PPCMessage>();
        int64_t length = msg->decode(std::move(_buffer));
        if (length == -1)
        {
            return nullptr;
        }
        return msg;
    }

    PPCMessageFace::Ptr decodePPCMessage(ppc::protocol::Message::Ptr msg) override;

    ppc::protocol::Message::Ptr buildMessage(ppc::protocol::MessageBuilder::Ptr const& msgBuilder,
        ppc::protocol::MessagePayloadBuilder::Ptr const& msgPayloadBuilder,
        PPCMessageFace::Ptr const& ppcMessage) override;

    ppc::protocol::MessagePayload::Ptr buildMessage(
        ppc::protocol::MessagePayloadBuilder::Ptr const& msgPayloadBuilder,
        PPCMessageFace::Ptr const& ppcMessage) override;
};

}  // namespace front
}  // namespace ppc
