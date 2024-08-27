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
#include "ppc-framework/protocol/Message.h"
#include "ppc-framework/protocol/RouteType.h"

namespace ppc::gateway
{
class LocalRouter
{
public:
    using Ptr = std::shared_ptr<LocalRouter>;
    LocalRouter(GatewayNodeInfoFactory::Ptr nodeInfoFactory,
        ppc::front::IFrontBuilder::Ptr frontBuilder, MessageCache::Ptr msgCache)
      : m_routerInfo(std::move(nodeInfoFactory->build())),
        m_frontBuilder(std::move(frontBuilder)),
        m_cache(std::move(msgCache))
    {}

    virtual ~LocalRouter() = default;

    virtual bool registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo)
    {
        nodeInfo->setFront(m_frontBuilder->buildClient(nodeInfo->endPoint()));
        return m_routerInfo->tryAddNodeInfo(nodeInfo);
    }

    virtual void unRegisterNode(bcos::bytes const& nodeID) { m_routerInfo->removeNodeInfo(nodeID); }

    virtual void registerTopic(bcos::bytesConstRef nodeID, std::string const& topic);
    virtual void unRegisterTopic(bcos::bytesConstRef nodeID, std::string const& topic);

    virtual std::vector<ppc::front::IFront::Ptr> chooseReceiver(
        ppc::protocol::Message::Ptr const& msg);

    // TODO: register component
    virtual bool dispatcherMessage(ppc::protocol::Message::Ptr const& msg,
        ppc::protocol::ReceiveMsgFunc callback, bool holding = true);

private:
    ppc::front::IFrontBuilder::Ptr m_frontBuilder;
    GatewayNodeInfo::Ptr m_routerInfo;

    // NodeID=>topics
    using Topics = std::set<std::string>;
    std::map<bcos::bytes, Topics> m_topicInfo;
    mutable bcos::SharedMutex x_topicInfo;

    MessageCache::Ptr m_cache;
};
}  // namespace ppc::gateway