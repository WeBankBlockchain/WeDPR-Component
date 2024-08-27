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
#include "LocalRouter.h"
#include "ppc-framework/Common.h"
#include "ppc-framework/gateway/GatewayProtocol.h"

using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::gateway;

void LocalRouter::registerTopic(bcos::bytesConstRef _nodeID, std::string const& topic)
{
    m_routerInfo->registerTopic(_nodeID.toBytes(), topic);
    // try to dispatch the cacheInfo
    if (!m_cache)
    {
        return;
    }
    auto msgQueue = m_cache->pop(topic);
    if (!msgQueue)
    {
        return;
    }
    if (msgQueue->timer)
    {
        msgQueue->timer->cancel();
    }
    for (auto const& msgInfo : msgQueue->messages)
    {
        dispatcherMessage(msgInfo.msg, msgInfo.callback, false);
    }
}

void LocalRouter::unRegisterTopic(bcos::bytesConstRef _nodeID, std::string const& topic)
{
    m_routerInfo->unRegisterTopic(_nodeID.toBytes(), topic);
}

bool LocalRouter::dispatcherMessage(Message::Ptr const& msg, ReceiveMsgFunc callback, bool holding)
{
    auto frontList = chooseReceiver(msg);
    // send success
    if (!frontList.empty())
    {
        for (auto const& front : frontList)
        {
            front->onReceiveMessage(msg, callback);
        }
        return true;
    }
    if (!holding)
    {
        return false;
    }
    // no connection found, cache the topic message and dispatcher later
    if (msg->header()->routeType() == (uint16_t)RouteType::ROUTE_THROUGH_TOPIC && m_cache)
    {
        m_cache->insertCache(msg->header()->optionalField()->topic(), msg, callback);
        return true;
    }
    return false;
}

std::vector<ppc::front::IFront::Ptr> LocalRouter::chooseReceiver(
    ppc::protocol::Message::Ptr const& msg)
{
    std::vector<ppc::front::IFront::Ptr> receivers;
    if (msg->header()->optionalField()->dstInst() != m_routerInfo->agency())
    {
        return receivers;
    }
    bool selectAll =
        (msg->header()->packetType() == (uint16_t)GatewayPacketType::BroadcastMessage ? true :
                                                                                        false);
    switch (msg->header()->routeType())
    {
    case (uint16_t)RouteType::ROUTE_THROUGH_NODEID:
    {
        auto gatewayInfo = m_routerInfo->nodeInfo(msg->header()->optionalField()->dstNode());
        if (gatewayInfo != nullptr)
        {
            receivers.emplace_back(gatewayInfo->getFront());
        }
        return receivers;
    }
    case (uint16_t)RouteType::ROUTE_THROUGH_COMPONENT:
    {
        return m_routerInfo->chooseRouteByComponent(
            selectAll, msg->header()->optionalField()->componentType());
    }
    case (uint16_t)RouteType::ROUTE_THROUGH_AGENCY:
    {
        return m_routerInfo->chooseRouterByAgency(selectAll);
    }
    case (uint16_t)RouteType::ROUTE_THROUGH_TOPIC:
    {
        return m_routerInfo->chooseRouterByTopic(
            selectAll, msg->header()->optionalField()->topic());
    }
    default:
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "chooseReceiver failed for unknown routeType, message detail: " +
                                  printMessage(msg)));
    }
}
