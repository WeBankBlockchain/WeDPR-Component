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
 * @date 2024-08-22
 */
#pragma once
#include "ppc-framework/protocol/P2PMessage.h"
#include "ppc-utilities/Utilities.h"

namespace ppc::protocol
{
class P2PMessageImpl : public P2PMessage
{
public:
    using Ptr = std::shared_ptr<P2PMessageImpl>;
    P2PMessageImpl(Message::Ptr msg) : P2PMessage(msg) {}
    ~P2PMessageImpl() override = default;
    // encode and return the {header, payload}
    bool encode(bcos::boostssl::EncodedMsg& _encodedMsg) override;
};

class P2PMessageBuilderImpl : public P2PMessageBuilder
{
public:
    using Ptr = std::shared_ptr<P2PMessageBuilderImpl>;
    P2PMessageBuilderImpl(MessageBuilder::Ptr msgBuilder) : m_msgBuilder(std::move(msgBuilder)) {}
    ~P2PMessageBuilderImpl() override = default;

    P2PMessage::Ptr build() override
    {
        return std::make_shared<P2PMessageImpl>(m_msgBuilder->build());
    }

    P2PMessage::Ptr build(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload) override
    {
        return std::make_shared<P2PMessageImpl>(
            m_msgBuilder->build(routeType, routeInfo, std::move(payload)));
    }

    bcos::boostssl::MessageFace::Ptr buildMessage() override
    {
        return std::make_shared<P2PMessageImpl>(m_msgBuilder->build());
    }
    std::string newSeq() override { return ppc::generateUUID(); }

protected:
    MessageBuilder::Ptr m_msgBuilder;
};
}  // namespace ppc::protocol