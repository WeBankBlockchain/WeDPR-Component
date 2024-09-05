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
 * @file MessageHeaderImpl.h
 * @author: yujiechen
 * @date 2024-08-23
 */
#pragma once
#include "ppc-framework/gateway/GatewayProtocol.h"
#include "ppc-framework/protocol/Message.h"

namespace ppc::protocol
{
class MessageOptionalHeaderImpl : public MessageOptionalHeader
{
public:
    using Ptr = std::shared_ptr<MessageOptionalHeaderImpl>;
    MessageOptionalHeaderImpl() = default;
    MessageOptionalHeaderImpl(MessageOptionalHeader::Ptr const& optionalHeader)
    {
        if (!optionalHeader)
        {
            return;
        }
        setTopic(optionalHeader->topic());
        setComponentType(optionalHeader->componentType());
        setSrcNode(optionalHeader->srcNode());
        setDstNode(optionalHeader->dstNode());
        setDstInst(optionalHeader->dstInst());
    }
    MessageOptionalHeaderImpl(bcos::bytesConstRef data, uint64_t const offset)
    {
        decode(data, offset);
    }

    ~MessageOptionalHeaderImpl() override = default;

    void encode(bcos::bytes& buffer) const override;
    int64_t decode(bcos::bytesConstRef data, uint64_t const offset) override;
};

class MessageHeaderImpl : public MessageHeader
{
public:
    using Ptr = std::shared_ptr<MessageHeaderImpl>;
    MessageHeaderImpl() { m_optionalField = std::make_shared<MessageOptionalHeaderImpl>(); }
    MessageHeaderImpl(bcos::bytesConstRef data)
    {
        m_optionalField = std::make_shared<MessageOptionalHeaderImpl>();
        decode(data);
    }
    ~MessageHeaderImpl() override {}

    void encode(bcos::bytes& buffer) const override;
    int64_t decode(bcos::bytesConstRef data) override;

    bool hasOptionalField() const override
    {
        return m_packetType == (uint16_t)ppc::gateway::GatewayPacketType::P2PMessage;
    }

    bool isRespPacket() const override
    {
        return m_ext & (uint16_t)ppc::gateway::GatewayMsgExtFlag::Response;
    }
    void setRespPacket() override { m_ext |= (uint16_t)ppc::gateway::GatewayMsgExtFlag::Response; }

    uint16_t routeType() const override;
    void setRouteType(ppc::protocol::RouteType type) override;

private:
    // version(2) + packetType(2) + ttl(2) + ext(2) + traceIDLen(2) + srcGwNodeLen(2) + dstGwNode(2)
    const size_t MESSAGE_MIN_LENGTH = 14;
};

class MessageHeaderBuilderImpl : public MessageHeaderBuilder
{
public:
    using Ptr = std::shared_ptr<MessageHeaderBuilderImpl>;
    MessageHeaderBuilderImpl() = default;
    ~MessageHeaderBuilderImpl() {}

    MessageHeader::Ptr build(bcos::bytesConstRef data) override
    {
        return std::make_shared<MessageHeaderImpl>(data);
    }
    MessageHeader::Ptr build() override { return std::make_shared<MessageHeaderImpl>(); }
    MessageOptionalHeader::Ptr build(MessageOptionalHeader::Ptr const& optionalHeader) override
    {
        return std::make_shared<MessageOptionalHeaderImpl>(optionalHeader);
    }
};
class MessageOptionalHeaderBuilderImpl : public MessageOptionalHeaderBuilder
{
public:
    using Ptr = std::shared_ptr<MessageOptionalHeaderBuilderImpl>;
    MessageOptionalHeaderBuilderImpl() = default;
    ~MessageOptionalHeaderBuilderImpl() override = default;

    MessageOptionalHeader::Ptr build(MessageOptionalHeader::Ptr const& optionalHeader) override
    {
        return std::make_shared<MessageOptionalHeaderImpl>(optionalHeader);
    }

    MessageOptionalHeader::Ptr build() override
    {
        return std::make_shared<MessageOptionalHeaderImpl>();
    }
};
}  // namespace ppc::protocol