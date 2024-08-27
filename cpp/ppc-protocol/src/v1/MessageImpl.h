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
 * @file MessageImpl.h
 * @author: yujiechen
 * @date 2024-08-23
 */
#pragma once
#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/Message.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace ppc::protocol
{
class MessageImpl : public Message
{
public:
    using Ptr = std::shared_ptr<Message>;
    MessageImpl(MessageHeaderBuilder::Ptr headerBuilder, size_t maxMessageLen)
      : m_headerBuilder(std::move(headerBuilder)), m_maxMessageLen(maxMessageLen)
    {}
    MessageImpl(
        MessageHeaderBuilder::Ptr headerBuilder, size_t maxMessageLen, bcos::bytesConstRef buffer)
      : MessageImpl(headerBuilder, maxMessageLen)
    {
        decode(buffer);
    }

    ~MessageImpl() override = default;

    bool encode(bcos::bytes& _buffer) override;
    // encode and return the {header, payload}
    bool encode(bcos::boostssl::EncodedMsg& _encodedMsg) override;
    int64_t decode(bcos::bytesConstRef _buffer) override;

private:
    MessageHeaderBuilder::Ptr m_headerBuilder;

    // default max message length is 100MB
    size_t m_maxMessageLen = 100 * 1024 * 1024;
};

class MessageBuilderImpl : public MessageBuilder
{
public:
    using Ptr = std::shared_ptr<MessageBuilderImpl>;
    MessageBuilderImpl(MessageHeaderBuilder::Ptr msgHeaderBuilder)
      : m_msgHeaderBuilder(std::move(msgHeaderBuilder))
    {}

    MessageBuilderImpl(MessageHeaderBuilder::Ptr msgHeaderBuilder, size_t maxMessageLen)
      : MessageBuilderImpl(std::move(msgHeaderBuilder))
    {
        m_maxMessageLen = maxMessageLen;
    }

    ~MessageBuilderImpl() override {}

    Message::Ptr build() override
    {
        return std::make_shared<MessageImpl>(m_msgHeaderBuilder, m_maxMessageLen);
    }
    Message::Ptr build(bcos::bytesConstRef buffer) override
    {
        return std::make_shared<MessageImpl>(m_msgHeaderBuilder, m_maxMessageLen, buffer);
    }
    Message::Ptr build(ppc::protocol::RouteType routeType, std::string const& topic,
        std::string const& dstInst, bcos::bytes const& dstNodeID, std::string const& componentType,
        bcos::bytes&& payload) override
    {
        auto msg = build();
        msg->header()->setRouteType(routeType);
        msg->header()->optionalField()->setDstInst(dstInst);
        msg->header()->optionalField()->setDstNode(dstNodeID);
        msg->header()->optionalField()->setTopic(topic);
        msg->header()->optionalField()->setComponentType(componentType);
        msg->setPayload(std::make_shared<bcos::bytes>(std::move(payload)));
        return msg;
    }

    bcos::boostssl::MessageFace::Ptr buildMessage() override
    {
        return std::make_shared<MessageImpl>(m_msgHeaderBuilder, m_maxMessageLen);
    }

    std::string newSeq() override
    {
        std::string seq = boost::uuids::to_string(boost::uuids::random_generator()());
        seq.erase(std::remove(seq.begin(), seq.end(), '-'), seq.end());
        return seq;
    }

private:
    MessageHeaderBuilder::Ptr m_msgHeaderBuilder;
    // default max message length is 100MB
    size_t m_maxMessageLen = 100 * 1024 * 1024;
};
}  // namespace ppc::protocol