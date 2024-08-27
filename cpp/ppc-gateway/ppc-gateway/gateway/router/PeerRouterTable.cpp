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
 * @file PeerRouterTable.cpp
 * @author: yujiechen
 * @date 2024-08-27
 */
#include "PeerRouterTable.h"
#include "ppc-framework/Common.h"
#include <random>

using namespace bcos;
using namespace ppc;
using namespace ppc::gateway;
using namespace ppc::protocol;

void PeerRouterTable::updateGatewayInfo(GatewayNodeInfo::Ptr const& gatewayInfo)
{
    auto nodeList = gatewayInfo->nodeList();
    bcos::WriteGuard l(x_mutex);
    for (auto const& it : nodeList)
    {
        // update nodeID => gatewayInfos
        m_nodeID2GatewayInfos[it.first].insert(gatewayInfo);
    }
    // update agency => gatewayInfos
    m_agency2GatewayInfos[gatewayInfo->agency()].insert(gatewayInfo);
}

GatewayNodeInfos PeerRouterTable::selectRouter(
    RouteType const& routeType, Message::Ptr const& msg) const
{
    switch (routeType)
    {
    case RouteType::ROUTE_THROUGH_NODEID:
        return selectRouterByNodeID(msg);
    case RouteType::ROUTE_THROUGH_COMPONENT:
        return selectRouterByComponent(msg);
    case RouteType::ROUTE_THROUGH_AGENCY:
    case RouteType::ROUTE_THROUGH_TOPIC:
        return selectRouterByAgency(msg);
    default:
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "selectRouter failed for encounter unsupported routeType: " +
                                  std::to_string((uint16_t)routeType)));
    }
}

GatewayNodeInfos PeerRouterTable::selectRouterByNodeID(Message::Ptr const& msg) const
{
    GatewayNodeInfos result;
    bcos::ReadGuard l(x_mutex);
    auto it = m_nodeID2GatewayInfos.find(msg->header()->optionalField()->dstNode());
    // no router found
    if (it == m_nodeID2GatewayInfos.end())
    {
        return result;
    }
    return it->second;
}


GatewayNodeInfos PeerRouterTable::selectRouterByAgency(Message::Ptr const& msg) const
{
    GatewayNodeInfos result;
    bcos::ReadGuard l(x_mutex);
    auto it = m_agency2GatewayInfos.find(msg->header()->optionalField()->dstInst());
    // no router found
    if (it == m_agency2GatewayInfos.end())
    {
        return result;
    }
    return it->second;
}

GatewayNodeInfos PeerRouterTable::selectRouterByComponent(Message::Ptr const& msg) const
{
    GatewayNodeInfos result;
    bcos::ReadGuard l(x_mutex);
    auto it = m_agency2GatewayInfos.find(msg->header()->optionalField()->dstInst());
    // no router found
    if (it == m_agency2GatewayInfos.end())
    {
        return result;
    }
    auto const& gatewayInfos = it->second;
    // foreach all gateways to find the component
    for (auto const& it : gatewayInfos)
    {
        auto const& nodeListInfo = it->nodeList();
        for (auto const& nodeInfo : nodeListInfo)
        {
            if (nodeInfo.second->components().count(
                    msg->header()->optionalField()->componentType()))
            {
                result.insert(it);
                break;
            }
        }
    }
    return result;
}

void PeerRouterTable::asyncBroadcastMessage(ppc::protocol::Message::Ptr const& msg) const
{
    bcos::ReadGuard l(x_mutex);
    for (auto const& it : m_agency2GatewayInfos)
    {
        auto selectedIndex = rand() % it.second.size();
        auto iterator = it.second.begin();
        if (selectedIndex > 0)
        {
            std::advance(iterator, selectedIndex);
        }
        auto selectedNode = *iterator;
        // ignore self
        if (selectedNode->p2pNodeID() == m_service->nodeID())
        {
            continue;
        }
        GATEWAY_LOG(TRACE) << LOG_DESC("asyncBroadcastMessage")
                           << LOG_KV("nodeID", printP2PIDElegantly(selectedNode->p2pNodeID()))
                           << LOG_KV("msg", printMessage(msg));
        m_service->asyncSendMessageByNodeID(selectedNode->p2pNodeID(), msg);
    }
}
