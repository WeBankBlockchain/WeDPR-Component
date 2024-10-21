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
#include "ppc-framework/Helper.h"
#include <random>

using namespace bcos;
using namespace ppc;
using namespace ppc::gateway;
using namespace ppc::protocol;

void PeerRouterTable::updateGatewayInfo(GatewayNodeInfo::Ptr const& gatewayInfo)
{
    PEER_ROUTER_LOG(INFO) << LOG_DESC("updateGatewayInfo")
                          << LOG_KV("detail", printNodeStatus(gatewayInfo));
    auto nodeList = gatewayInfo->nodeList();

    removeP2PNodeIDFromNodeIDInfos(gatewayInfo);
    removeP2PNodeIDFromAgencyInfos(gatewayInfo->p2pNodeID());
    insertGatewayInfo(gatewayInfo);
}

void PeerRouterTable::insertGatewayInfo(GatewayNodeInfo::Ptr const& gatewayInfo)
{
    auto nodeList = gatewayInfo->nodeList();
    bcos::WriteGuard l(x_mutex);
    // insert new information for the gateway
    for (auto const& it : nodeList)
    {
        // update nodeID => gatewayInfos
        if (!m_nodeID2GatewayInfos.count(it.first))
        {
            m_nodeID2GatewayInfos.insert(std::make_pair(it.first, GatewayNodeInfos()));
        }
        m_nodeID2GatewayInfos[it.first].insert(gatewayInfo);
    }
    if (!m_agency2GatewayInfos.count(gatewayInfo->agency()))
    {
        m_agency2GatewayInfos.insert(std::make_pair(gatewayInfo->agency(), GatewayNodeInfos()));
    }
    // update agency => gatewayInfos
    m_agency2GatewayInfos[gatewayInfo->agency()].insert(gatewayInfo);
}

void PeerRouterTable::removeP2PNodeIDFromNodeIDInfos(GatewayNodeInfo::Ptr const& gatewayInfo)
{
    bcos::WriteGuard l(x_mutex);
    // remove the origin information of the gateway
    auto it = m_nodeID2GatewayInfos.begin();
    for (; it != m_nodeID2GatewayInfos.end();)
    {
        auto& gatewayInfos = it->second;
        auto ptr = gatewayInfos.find(gatewayInfo);
        if (ptr != gatewayInfos.end())
        {
            gatewayInfos.erase(ptr);
        }
        if (gatewayInfos.empty())
        {
            it = m_nodeID2GatewayInfos.erase(it);
            continue;
        }
        it++;
    }
}

void PeerRouterTable::removeP2PNodeIDFromAgencyInfos(std::string const& p2pNode)
{
    bcos::WriteGuard l(x_mutex);
    for (auto it = m_agency2GatewayInfos.begin(); it != m_agency2GatewayInfos.end();)
    {
        auto& gatewayInfos = it->second;
        for (auto pGateway = gatewayInfos.begin(); pGateway != gatewayInfos.end();)
        {
            if ((*pGateway)->p2pNodeID() == p2pNode)
            {
                pGateway = gatewayInfos.erase(pGateway);
                continue;
            }
            pGateway++;
        }
        if (gatewayInfos.empty())
        {
            it = m_agency2GatewayInfos.erase(it);
            continue;
        }
        it++;
    }
}

void PeerRouterTable::removeP2PID(std::string const& p2pNode)
{
    PEER_ROUTER_LOG(INFO) << LOG_DESC("PeerRouterTable: removeP2PID")
                          << LOG_KV("p2pID", printP2PIDElegantly(p2pNode));
    // remove P2PNode from m_nodeID2GatewayInfos
    auto gatewayInfo = m_gatewayInfoFactory->build(p2pNode);
    removeP2PNodeIDFromNodeIDInfos(gatewayInfo);
    // remove P2PNode from m_agency2GatewayInfos
    removeP2PNodeIDFromAgencyInfos(p2pNode);
}

std::set<std::string> PeerRouterTable::agencies(std::vector<std::string> const& components) const
{
    std::set<std::string> agencies;
    bcos::ReadGuard l(x_mutex);
    for (auto const& it : m_agency2GatewayInfos)
    {
        // get all agencies
        if (components.empty())
        {
            agencies.insert(it.first);
            continue;
        }
        // get agencies according to component
        for (auto const& gatewayInfo : it.second)
        {
            for (auto const& component : components)
            {
                if (gatewayInfo->existComponent(component))
                {
                    agencies.insert(it.first);
                    break;
                }
            }
        }
    }
    return agencies;
}

GatewayNodeInfos PeerRouterTable::selectRouter(
    RouteType const& routeType, MessageOptionalHeader::Ptr const& routeInfo) const
{
    switch (routeType)
    {
    case RouteType::ROUTE_THROUGH_NODEID:
        return selectRouterByNodeID(routeInfo);
    case RouteType::ROUTE_THROUGH_COMPONENT:
        return selectRouterByComponent(routeInfo);
    case RouteType::ROUTE_THROUGH_AGENCY:
    case RouteType::ROUTE_THROUGH_TOPIC:
        return selectRouterByAgency(routeInfo);
    default:
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "selectRouter failed for encounter unsupported routeType: " +
                                  std::to_string((uint16_t)routeType)));
    }
}

std::vector<std::string> PeerRouterTable::selectTargetNodes(
    RouteType const& routeType, MessageOptionalHeader::Ptr const& routeInfo) const
{
    std::set<std::string> targetNodeList;
    auto selectedP2PNodes = selectRouter(routeType, routeInfo);
    if (selectedP2PNodes.empty())
    {
        PEER_ROUTER_LOG(INFO) << LOG_DESC("selectTargetNodes with empty result")
                              << LOG_KV("routeType", routeType)
                              << LOG_KV("routeInfo", printOptionalField(routeInfo));
        return std::vector<std::string>();
    }
    for (auto const& it : selectedP2PNodes)
    {
        auto nodeList = it->nodeList();
        for (auto const& it : nodeList)
        {
            if (routeType == RouteType::ROUTE_THROUGH_COMPONENT)
            {
                if (it.second->componentExist(routeInfo->componentType()))
                {
                    targetNodeList.insert(std::string(it.first.begin(), it.first.end()));
                }
                continue;
            }
            targetNodeList.insert(std::string(it.first.begin(), it.first.end()));
        }
    }
    PEER_ROUTER_LOG(INFO) << LOG_DESC("selectTargetNodes, result: ")
                          << printCollection(targetNodeList) << LOG_KV("routeType", routeType)
                          << LOG_KV("routeInfo", printOptionalField(routeInfo));
    return std::vector<std::string>(targetNodeList.begin(), targetNodeList.end());
}

GatewayNodeInfos PeerRouterTable::selectRouterByNodeID(
    MessageOptionalHeader::Ptr const& routeInfo) const
{
    GatewayNodeInfos result;
    bcos::ReadGuard l(x_mutex);
    auto it = m_nodeID2GatewayInfos.find(routeInfo->dstNode());
    // no router found
    if (it == m_nodeID2GatewayInfos.end())
    {
        return result;
    }
    return it->second;
}


GatewayNodeInfos PeerRouterTable::selectRouterByAgency(
    MessageOptionalHeader::Ptr const& routeInfo) const
{
    GatewayNodeInfos result;
    bcos::ReadGuard l(x_mutex);
    auto it = m_agency2GatewayInfos.find(routeInfo->dstInst());
    // no router found
    if (it == m_agency2GatewayInfos.end())
    {
        return result;
    }
    return it->second;
}

// Note: selectRouterByComponent support not specified the dstInst
GatewayNodeInfos PeerRouterTable::selectRouterByComponent(
    MessageOptionalHeader::Ptr const& routeInfo) const
{
    GatewayNodeInfos result;
    auto dstInst = routeInfo->dstInst();
    std::vector<GatewayNodeInfos> selectedRouterInfos;
    {
        bcos::ReadGuard l(x_mutex);
        if (dstInst.size() > 0)
        {
            // specified the dstInst
            auto it = m_agency2GatewayInfos.find(dstInst);
            // no router found
            if (it == m_agency2GatewayInfos.end())
            {
                return result;
            }
            selectedRouterInfos.emplace_back(it->second);
        }
        else
        {
            // the dstInst not specified, query from all agencies
            for (auto const& it : m_agency2GatewayInfos)
            {
                selectedRouterInfos.emplace_back(it.second);
            }
        }
    }
    for (auto const& it : selectedRouterInfos)
    {
        selectRouterByComponent(result, routeInfo, it);
    }
    return result;
}


void PeerRouterTable::selectRouterByComponent(GatewayNodeInfos& choosedGateway,
    MessageOptionalHeader::Ptr const& routeInfo,
    GatewayNodeInfos const& singleAgencyGatewayInfos) const
{
    // foreach all gateways to find the component
    for (auto const& it : singleAgencyGatewayInfos)
    {
        auto const& nodeListInfo = it->nodeList();
        for (auto const& nodeInfo : nodeListInfo)
        {
            if (nodeInfo.second->componentExist(routeInfo->componentType()))
            {
                choosedGateway.insert(it);
                break;
            }
        }
    }
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
        PEER_ROUTER_LOG(TRACE) << LOG_DESC("asyncBroadcastMessage")
                               << LOG_KV("nodeID", printP2PIDElegantly(selectedNode->p2pNodeID()))
                               << LOG_KV("msg", printMessage(msg));
        m_service->asyncSendMessageByNodeID(selectedNode->p2pNodeID(), msg);
    }
}
