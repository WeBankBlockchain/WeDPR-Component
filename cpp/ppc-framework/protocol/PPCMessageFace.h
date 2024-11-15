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
 * @file PPCMessageFace.h
 * @author: shawnhe
 * @date 2022-10-19
 */

#pragma once
#include "Protocol.h"
#include "ppc-framework/protocol/Message.h"
#include <bcos-utilities/Common.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>
#include <string>

namespace ppc
{
namespace front
{
class PPCMessageFace
{
public:
    using Ptr = std::shared_ptr<PPCMessageFace>;

public:
    virtual ~PPCMessageFace() {}

    virtual uint8_t version() const = 0;
    virtual void setVersion(uint8_t) = 0;
    virtual uint8_t taskType() const = 0;
    virtual void setTaskType(uint8_t) = 0;
    virtual uint8_t algorithmType() const = 0;
    virtual void setAlgorithmType(uint8_t) = 0;
    virtual uint8_t messageType() const = 0;
    virtual void setMessageType(uint8_t) = 0;
    virtual uint32_t seq() const = 0;
    virtual void setSeq(uint32_t) = 0;
    virtual std::string const& taskID() const = 0;
    virtual void setTaskID(std::string const&) = 0;
    virtual std::string const& sender() const = 0;
    virtual bcos::bytes const& senderNode() const = 0;
    virtual void setSender(std::string const&) = 0;
    virtual void setSenderNode(bcos::bytes const&) = 0;

    virtual std::shared_ptr<bcos::bytes> data() const = 0;
    virtual void setData(std::shared_ptr<bcos::bytes>) = 0;
    virtual std::map<std::string, std::string> header() = 0;
    virtual void setHeader(std::map<std::string, std::string>) = 0;
    virtual void encode(bcos::bytes& _buffer) = 0;
    virtual int64_t decode(bcos::bytesPointer _buffer) = 0;
    virtual int64_t decode(bcos::bytesConstRef _buffer) = 0;

    virtual uint32_t length() const = 0;

    virtual std::string uuid() const = 0;
    virtual void setUuid(std::string const& _uuid) = 0;

    // determine the message is response or not
    virtual bool response() const = 0;
    // set the message to be response
    virtual void setResponse() = 0;

    virtual void releasePayload() {}
};

class PPCMessageFaceFactory
{
public:
    using Ptr = std::shared_ptr<PPCMessageFaceFactory>;

public:
    virtual ~PPCMessageFaceFactory() {}
    virtual PPCMessageFace::Ptr buildPPCMessage() = 0;
    virtual PPCMessageFace::Ptr decodePPCMessage(ppc::protocol::Message::Ptr msg) = 0;
    virtual ppc::protocol::Message::Ptr buildMessage(
        ppc::protocol::MessageBuilder::Ptr const& msgBuilder,
        ppc::protocol::MessagePayloadBuilder::Ptr const& msgPayloadBuilder,
        PPCMessageFace::Ptr const& ppcMessage) = 0;

    virtual ppc::protocol::MessagePayload::Ptr buildMessage(
        ppc::protocol::MessagePayloadBuilder::Ptr const& msgPayloadBuilder,
        PPCMessageFace::Ptr const& ppcMessage) = 0;

    virtual PPCMessageFace::Ptr buildPPCMessage(bcos::bytesConstRef _data) = 0;
    virtual PPCMessageFace::Ptr buildPPCMessage(bcos::bytesPointer _buffer) = 0;
    virtual PPCMessageFace::Ptr buildPPCMessage(uint8_t _taskType, uint8_t _algorithmType,
        std::string const& _taskID, bcos::bytesPointer _data) = 0;
};

inline std::string printPPCMsg(PPCMessageFace::Ptr _msg)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("sender", _msg->sender())
                 << LOG_KV("taskType", (ppc::protocol::TaskType)(_msg->taskType()))
                 << LOG_KV("algorithm", int(_msg->algorithmType()))
                 << LOG_KV("messageType", int(_msg->messageType()))
                 << LOG_KV("task", _msg->taskID()) << LOG_KV("seq", _msg->seq())
                 << LOG_KV("uuid", _msg->uuid()) << LOG_KV("response", _msg->response())
                 << LOG_KV("size", _msg->data()->size());
    return stringstream.str();
}
}  // namespace front
}  // namespace ppc
