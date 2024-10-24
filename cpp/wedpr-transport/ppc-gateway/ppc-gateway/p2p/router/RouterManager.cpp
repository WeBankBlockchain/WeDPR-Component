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
 * @file RouterManager.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */
#include "RouterManager.h"
#include "ppc-framework/Helper.h"
#include "ppc-framework/gateway/GatewayProtocol.h"
#include "ppc-framework/protocol/P2PMessage.h"
#include <boost/asio/detail/socket_ops.hpp>

using namespace bcos;
using namespace bcos::boostssl::ws;
using namespace bcos::boostssl;
using namespace ppc::gateway;
using namespace ppc::protocol;

RouterManager::RouterManager(Service::Ptr service) : m_service(std::move(service))
{
    // process router packet related logic
    m_service->registerMsgHandler((uint16_t)GatewayPacketType::RouterTableSyncSeq,
        boost::bind(&RouterManager::onReceiveRouterSeq, this, boost::placeholders::_1,
            boost::placeholders::_2));

    m_service->registerMsgHandler((uint16_t)GatewayPacketType::RouterTableResponse,
        boost::bind(&RouterManager::onReceivePeersRouterTable, this, boost::placeholders::_1,
            boost::placeholders::_2));

    m_service->registerMsgHandler((uint16_t)GatewayPacketType::RouterTableRequest,
        boost::bind(&RouterManager::onReceiveRouterTableRequest, this, boost::placeholders::_1,
            boost::placeholders::_2));
    m_routerTimer = std::make_shared<bcos::Timer>(3000, "routerSeqSync");
    m_routerTimer->registerTimeoutHandler([this]() { broadcastRouterSeq(); });
}

void RouterManager::start()
{
    if (m_routerTimer)
    {
        m_routerTimer->start();
    }
}

void RouterManager::stop()
{
    if (m_routerTimer)
    {
        m_routerTimer->stop();
    }
}

void RouterManager::onReceiveRouterSeq(MessageFace::Ptr msg, WsSession::Ptr session)
{
    auto statusSeq =
        boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)msg->payload()->data()));
    if (!tryToUpdateSeq(session->nodeId(), statusSeq))
    {
        return;
    }
    SERVICE_ROUTER_LOG(INFO) << LOG_BADGE("onReceiveRouterSeq")
                             << LOG_DESC("receive router seq and request router table")
                             << LOG_KV("peer", printP2PIDElegantly(session->nodeId()))
                             << LOG_KV("seq", statusSeq);
    // request router table to peer
    auto p2pMsg = std::dynamic_pointer_cast<P2PMessage>(msg);
    auto dstP2PNodeID = (!p2pMsg->header()->srcGwNode().empty()) ? p2pMsg->header()->srcGwNode() :
                                                                   session->nodeId();
    m_service->asyncSendMessageByP2PNodeID((uint16_t)GatewayPacketType::RouterTableRequest,
        dstP2PNodeID, std::make_shared<bcos::bytes>());
}

bool RouterManager::tryToUpdateSeq(std::string const& _p2pNodeID, uint32_t _seq)
{
    UpgradableGuard l(x_node2Seq);
    auto it = m_node2Seq.find(_p2pNodeID);
    if (it != m_node2Seq.end() && it->second >= _seq)
    {
        return false;
    }
    UpgradeGuard upgradeGuard(l);
    m_node2Seq[_p2pNodeID] = _seq;
    return true;
}

// receive routerTable from peers
void RouterManager::onReceivePeersRouterTable(MessageFace::Ptr msg, WsSession::Ptr session)
{
    auto routerTable = m_service->routerTableFactory()->createRouterTable(ref(*(msg->payload())));

    SERVICE_ROUTER_LOG(INFO) << LOG_BADGE("onReceivePeersRouterTable")
                             << LOG_KV("peer", printP2PIDElegantly(session->nodeId()))
                             << LOG_KV("entrySize", routerTable->routerEntries().size());
    joinRouterTable(session->nodeId(), routerTable);
}

// receive routerTable request from peer
void RouterManager::onReceiveRouterTableRequest(MessageFace::Ptr msg, WsSession::Ptr session)
{
    SERVICE_ROUTER_LOG(INFO) << LOG_BADGE("onReceiveRouterTableRequest")
                             << LOG_KV("peer", printP2PIDElegantly(session->nodeId()))
                             << LOG_KV(
                                    "entrySize", m_service->routerTable()->routerEntries().size());

    auto routerTableData = std::make_shared<bytes>();
    m_service->routerTable()->encode(*routerTableData);
    auto p2pMsg = std::dynamic_pointer_cast<P2PMessage>(msg);
    auto dstP2PNodeID = (!p2pMsg->header()->srcGwNode().empty()) ? p2pMsg->header()->srcGwNode() :
                                                                   session->nodeId();
    m_service->asyncSendMessageByP2PNodeID(
        (uint16_t)GatewayPacketType::RouterTableResponse, dstP2PNodeID, routerTableData);
}

void RouterManager::joinRouterTable(
    std::string const& _generatedFrom, RouterTableInterface::Ptr _routerTable)
{
    std::set<std::string> unreachableNodes;
    bool updated = false;
    auto const& entries = _routerTable->routerEntries();
    for (auto const& it : entries)
    {
        auto entry = it.second;
        if (m_service->routerTable()->update(unreachableNodes, _generatedFrom, entry) && !updated)
        {
            updated = true;
        }
    }

    SERVICE_ROUTER_LOG(INFO) << LOG_BADGE("joinRouterTable") << LOG_DESC("create router entry")
                             << LOG_KV("dst", printP2PIDElegantly(_generatedFrom));

    auto entry = m_service->routerTableFactory()->createRouterEntry();
    entry->setDstNode(_generatedFrom);
    entry->setDistance(0);
    if (m_service->routerTable()->update(unreachableNodes, m_service->nodeID(), entry) && !updated)
    {
        updated = true;
    }
    if (!updated)
    {
        SERVICE_ROUTER_LOG(DEBUG) << LOG_BADGE("joinRouterTable")
                                  << LOG_DESC("router table not updated")
                                  << LOG_KV("dst", printP2PIDElegantly(_generatedFrom));
        return;
    }
    onP2PNodesUnreachable(unreachableNodes);
    m_statusSeq++;
    broadcastRouterSeq();
}


// called when the nodes become unreachable
void RouterManager::onP2PNodesUnreachable(std::set<std::string> const& _p2pNodeIDs)
{
    try
    {
        std::vector<std::function<void(std::string)>> handlers;
        {
            ReadGuard readGuard(x_unreachableHandlers);
            handlers = m_unreachableHandlers;
        }
        // TODO: async here
        for (auto const& node : _p2pNodeIDs)
        {
            for (auto const& it : m_unreachableHandlers)
            {
                it(node);
            }
        }
    }
    catch (std::exception const& e)
    {
        SERVICE_ROUTER_LOG(WARNING) << LOG_DESC("onP2PNodesUnreachable exception")
                                    << LOG_KV("error", boost::diagnostic_information(e));
    }
}

void RouterManager::broadcastRouterSeq()
{
    m_routerTimer->restart();

    auto seq = m_statusSeq.load();
    auto statusSeq = boost::asio::detail::socket_ops::host_to_network_long(seq);
    auto message = m_service->messageFactory()->buildMessage();
    message->setPacketType((uint16_t)GatewayPacketType::RouterTableSyncSeq);
    message->setPayload(std::make_shared<bytes>((byte*)&statusSeq, (byte*)&statusSeq + 4));
    // the router table should only exchange between neighbor
    m_service->broadcastMessage(message);
}

std::set<std::string> RouterManager::onEraseSession(std::string const& sessionNodeID)
{
    eraseSeq(sessionNodeID);
    std::set<std::string> unreachableNodes;
    if (m_service->routerTable()->erase(unreachableNodes, sessionNodeID))
    {
        m_statusSeq++;
        broadcastRouterSeq();
    }
    onP2PNodesUnreachable(unreachableNodes);
    SERVICE_ROUTER_LOG(INFO) << LOG_DESC("onEraseSession")
                             << LOG_KV("dst", printP2PIDElegantly(sessionNodeID));
    return unreachableNodes;
}

bool RouterManager::eraseSeq(std::string const& _p2pNodeID)
{
    UpgradableGuard l(x_node2Seq);
    if (!m_node2Seq.count(_p2pNodeID))
    {
        return false;
    }
    UpgradeGuard ul(l);
    m_node2Seq.erase(_p2pNodeID);
    return true;
}

std::set<std::string> RouterManager::onNewSession(std::string const& sessionNodeID)
{
    std::set<std::string> unreachableNodes;
    auto entry = m_service->routerTableFactory()->createRouterEntry();
    entry->setDstNode(sessionNodeID);
    entry->setDistance(0);
    if (!m_service->routerTable()->update(unreachableNodes, m_service->nodeID(), entry))
    {
        SERVICE_ROUTER_LOG(INFO) << LOG_DESC("onNewSession: RouterTable not changed")
                                 << LOG_KV("dst", printP2PIDElegantly(sessionNodeID));
        return unreachableNodes;
    }
    m_statusSeq++;
    broadcastRouterSeq();
    SERVICE_ROUTER_LOG(INFO) << LOG_DESC("onNewSession: update routerTable")
                             << LOG_KV("dst", printP2PIDElegantly(sessionNodeID));
    onP2PNodesUnreachable(unreachableNodes);
    return unreachableNodes;
}