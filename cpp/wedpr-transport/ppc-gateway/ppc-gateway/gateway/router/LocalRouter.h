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
 * @file LocalRouter.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "../cache/MessageCache.h"
#include "GatewayNodeInfo.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include "ppc-framework/protocol/P2PMessage.h"
#include "ppc-framework/protocol/RouteType.h"

namespace ppc::gateway
{
class LocalRouter
{
public:
    using Ptr = std::shared_ptr<LocalRouter>;
    LocalRouter(GatewayNodeInfoFactory::Ptr gatewayNodeInfoFactory,
        ppc::front::IFrontBuilder::Ptr frontBuilder, MessageCache::Ptr msgCache)
      : m_routerInfo(gatewayNodeInfoFactory->build()),
        m_frontBuilder(std::move(frontBuilder)),
        m_cache(std::move(msgCache))
    {}

    virtual ~LocalRouter() = default;

    virtual bool registerNodeInfo(ppc::protocol::INodeInfo::Ptr nodeInfo,
        std::function<void()> onUnHealthHandler, bool removeHandlerOnUnhealth);
    virtual void unRegisterNode(bcos::bytes const& nodeID)
    {
        m_routerInfo->removeNodeInfo(nodeID);
        increaseSeq();
    }

    virtual void registerTopic(bcos::bytesConstRef nodeID, std::string const& topic);
    virtual void unRegisterTopic(bcos::bytesConstRef nodeID, std::string const& topic);

    virtual std::vector<ppc::front::IFrontClient::Ptr> chooseReceiver(
        ppc::protocol::P2PMessage::Ptr const& msg);

    virtual bool dispatcherMessage(ppc::protocol::P2PMessage::Ptr const& msg,
        ppc::protocol::ReceiveMsgFunc callback, bool holding = true);

    std::shared_ptr<bcos::bytes> generateNodeStatus()
    {
        auto data = std::make_shared<bcos::bytes>();
        m_routerInfo->setStatusSeq(m_statusSeq);
        m_routerInfo->encode(*data);
        return data;
    }
    uint32_t statusSeq() { return m_statusSeq; }

    GatewayNodeInfo::Ptr const& routerInfo() const { return m_routerInfo; }

    uint32_t increaseSeq()
    {
        uint32_t statusSeq = ++m_statusSeq;
        return statusSeq;
    }

private:
    ppc::front::IFrontBuilder::Ptr m_frontBuilder;

    GatewayNodeInfo::Ptr m_routerInfo;

    std::atomic<uint32_t> m_statusSeq{1};

    // NodeID=>topics
    using Topics = std::set<std::string>;
    std::map<bcos::bytes, Topics> m_topicInfo;
    mutable bcos::SharedMutex x_topicInfo;

    MessageCache::Ptr m_cache;
};
}  // namespace ppc::gateway