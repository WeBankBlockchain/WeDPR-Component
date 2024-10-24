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
    {
        m_header = m_headerBuilder->build();
    }
    MessageImpl(
        MessageHeaderBuilder::Ptr headerBuilder, size_t maxMessageLen, bcos::bytesConstRef buffer)
      : MessageImpl(headerBuilder, maxMessageLen)
    {
        decode(buffer);
    }

    ~MessageImpl() override = default;

    bool encode(bcos::bytes& _buffer) override;
    int64_t decode(bcos::bytesConstRef _buffer) override;

protected:
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

    virtual MessageOptionalHeader::Ptr build(MessageOptionalHeader::Ptr const& optionalHeader)
    {
        return m_msgHeaderBuilder->build(optionalHeader);
    }

    Message::Ptr build(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload) override
    {
        auto msg = build();
        msg->header()->setRouteType(routeType);
        msg->header()->setOptionalField(routeInfo);
        msg->setPayload(std::make_shared<bcos::bytes>(std::move(payload)));
        return msg;
    }

protected:
    MessageHeaderBuilder::Ptr m_msgHeaderBuilder;
    // default max message length is 100MB
    size_t m_maxMessageLen = 100 * 1024 * 1024;
};
}  // namespace ppc::protocol
