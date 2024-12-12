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
#include "ppc-framework/Helper.h"
#include "ppc-framework/gateway/GatewayProtocol.h"
#include "ppc-gateway/Common.h"

using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::gateway;

bool LocalRouter::registerNodeInfo(ppc::protocol::INodeInfo::Ptr nodeInfo,
    std::function<void()> onUnHealthHandler, bool removeHandlerOnUnhealth)
{
    LOCAL_ROUTER_LOG(DEBUG) << LOG_DESC("registerNodeInfo") << printNodeInfo(nodeInfo);
    bool updated = false;
    auto ret = m_routerInfo->tryAddNodeInfo(nodeInfo, updated);
    if (ret)
    {
        // only create the frontClient when update
        nodeInfo->setFront(m_frontBuilder->buildClient(
            nodeInfo->endPoint(), onUnHealthHandler, removeHandlerOnUnhealth));
        LOCAL_ROUTER_LOG(INFO) << LOG_DESC("registerNodeInfo: update the node")
                               << printNodeInfo(nodeInfo);
    }
    // update the status if inserted or updated
    if (ret || updated)
    {
        increaseSeq();
    }
    return ret;
}
// Note: the change of the topic will not trigger router-update
void LocalRouter::registerTopic(bcos::bytesConstRef _nodeID, std::string const& topic)
{
    LOCAL_ROUTER_LOG(INFO) << LOG_DESC("registerTopic") << LOG_KV("topic", topic)
                           << LOG_KV("nodeID", printNodeID(_nodeID));
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
        LOCAL_ROUTER_LOG(INFO) << LOG_DESC("registerTopic, dispatcher the holding msg queue")
                               << LOG_KV("topic", topic) << LOG_KV("nodeID", printNodeID(_nodeID));
        dispatcherMessage(msgInfo.msg, msgInfo.callback, false);
    }
}

// Note: the change of the topic will not trigger router-update
void LocalRouter::unRegisterTopic(bcos::bytesConstRef _nodeID, std::string const& topic)
{
    LOCAL_ROUTER_LOG(INFO) << LOG_DESC("unRegisterTopic") << LOG_KV("topic", topic)
                           << LOG_KV("nodeID", printNodeID(_nodeID));
    m_routerInfo->unRegisterTopic(_nodeID.toBytes(), topic);
}

bool LocalRouter::dispatcherMessage(
    P2PMessage::Ptr const& msg, ReceiveMsgFunc callback, bool holding)
{
    auto frontList = chooseReceiver(msg);
    auto commonCallback = [](bcos::Error::Ptr error) {
        if (!error || error->errorCode() == 0)
        {
            return;
        }
        LOCAL_ROUTER_LOG(WARNING) << LOG_DESC("dispatcherMessage to front failed")
                                  << LOG_KV("code", error->errorCode())
                                  << LOG_KV("msg", error->errorMessage());
    };
    // find the front
    if (!frontList.empty())
    {
        // Note: the callback can only been called once since it binds the callback seq
        int i = 0;
        for (auto const& front : frontList)
        {
            if (!front)
            {
                continue;
            }
            if (i == 0)
            {
                front->onReceiveMessage(msg->msg(), callback);
            }
            else
            {
                front->onReceiveMessage(msg->msg(), commonCallback);
            }
            i++;
        }
        return true;
    }
    // the case broadcast failed
    if (msg->header() && msg->header()->optionalField() &&
        msg->header()->optionalField()->topic().empty())
    {
        LOCAL_ROUTER_LOG(WARNING) << LOG_DESC("dispatcherMessage failed for no target found!")
                                  << printP2PMessage(msg);
        return false;
    }
    if (!holding)
    {
        return false;
    }
    // no connection found, cache the topic message and dispatcher later
    if (msg->header()->routeType() == (uint16_t)RouteType::ROUTE_THROUGH_TOPIC && m_cache)
    {
        // send response when hodling the message
        if (callback)
        {
            callback(nullptr);
        }
        m_cache->insertCache(msg->header()->optionalField()->topic(), msg, commonCallback);
        return true;
    }
    return false;
}

std::vector<ppc::front::IFrontClient::Ptr> LocalRouter::chooseReceiver(
    ppc::protocol::P2PMessage::Ptr const& msg)
{
    std::vector<ppc::front::IFrontClient::Ptr> receivers;
    auto const& dstInst = msg->header()->optionalField()->dstInst();
    if (!dstInst.empty() && dstInst != m_routerInfo->agency())
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
        auto front = gatewayInfo ? gatewayInfo->getFront() : nullptr;
        if (gatewayInfo != nullptr && front)
        {
            receivers.emplace_back(front);
        }
        return receivers;
    }
    case (uint16_t)RouteType::ROUTE_THROUGH_COMPONENT:
    {
        // Note: should check the dstInst when route-by-component
        return m_routerInfo->chooseRouteByComponent(
            selectAll, msg->header()->optionalField()->componentType());
    }
    case (uint16_t)RouteType::ROUTE_THROUGH_AGENCY:
    {
        // Note: should check the dstInst when route-by-agency
        return m_routerInfo->chooseRouterByAgency(selectAll);
    }
    case (uint16_t)RouteType::ROUTE_THROUGH_TOPIC:
    {
        // Note: should ignore the srcNode when route-by-topic
        return m_routerInfo->chooseRouterByTopic(selectAll,
            msg->header()->optionalField()->srcNode(), msg->header()->optionalField()->topic());
    }
    default:
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "chooseReceiver failed for unknown routeType, message detail: " +
                                  printP2PMessage(msg)));
    }
}
