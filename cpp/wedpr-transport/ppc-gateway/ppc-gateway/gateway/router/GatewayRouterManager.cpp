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
 * @file GatewayRouterManager.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#include "GatewayRouterManager.h"
#include "ppc-framework/Helper.h"
#include "ppc-framework/gateway/GatewayProtocol.h"
#include <ppc-gateway/Common.h>

using namespace ppc::protocol;
using namespace ppc;
using namespace bcos;
using namespace ppc::gateway;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;

GatewayRouterManager::GatewayRouterManager(Service::Ptr service,
    GatewayNodeInfoFactory::Ptr nodeStatusFactory, LocalRouter::Ptr localRouter,
    PeerRouterTable::Ptr peerRouter, uint16_t seqSyncPeriod)
  : m_service(std::move(service)),
    m_nodeStatusFactory(std::move(nodeStatusFactory)),
    m_localRouter(std::move(localRouter)),
    m_peerRouter(std::move(peerRouter)),
    m_seqSyncPeriod(seqSyncPeriod)
{
    m_service->registerMsgHandler((uint16_t)GatewayPacketType::SyncNodeSeq,
        boost::bind(&GatewayRouterManager::onReceiveNodeSeqMessage, this, boost::placeholders::_1,
            boost::placeholders::_2));

    m_service->registerMsgHandler((uint16_t)GatewayPacketType::RequestNodeStatus,
        boost::bind(&GatewayRouterManager::onReceiveRequestNodeStatusMsg, this,
            boost::placeholders::_1, boost::placeholders::_2));

    m_service->registerMsgHandler((uint16_t)GatewayPacketType::ResponseNodeStatus,
        boost::bind(&GatewayRouterManager::onRecvResponseNodeStatusMsg, this,
            boost::placeholders::_1, boost::placeholders::_2));

    m_timer = std::make_shared<Timer>(m_seqSyncPeriod, "seqSync");
    // broadcast seq periodically
    m_timer->registerTimeoutHandler([this]() { broadcastStatusSeq(); });
}


void GatewayRouterManager::start()
{
    if (m_running)
    {
        ROUTER_MGR_LOG(INFO) << LOG_DESC("GatewayRouterManager has already been started");
        return;
    }
    m_running = true;
    m_timer->start();
    ROUTER_MGR_LOG(INFO) << LOG_DESC("start GatewayRouterManager success");
}

void GatewayRouterManager::stop()
{
    if (!m_running)
    {
        ROUTER_MGR_LOG(INFO) << LOG_DESC("GatewayRouterManager has already been stopped");
        return;
    }
    m_running = false;
    m_timer->stop();
    ROUTER_MGR_LOG(INFO) << LOG_DESC("stop GatewayRouterManager success");
}

void GatewayRouterManager::removeUnreachableP2pNode(std::string const& p2pNode)
{
    ROUTER_MGR_LOG(INFO) << LOG_DESC("removeUnreachableP2pNode")
                         << LOG_KV("p2pid", printP2PIDElegantly(p2pNode));
    {
        // remove statusSeq info
        WriteGuard l(x_p2pID2Seq);
        m_p2pID2Seq.erase(p2pNode);
    }
    m_peerRouter->removeP2PID(p2pNode);
}

void GatewayRouterManager::onReceiveNodeSeqMessage(MessageFace::Ptr msg, WsSession::Ptr session)
{
    auto statusSeq =
        boost::asio::detail::socket_ops::network_to_host_long(*((uint32_t*)msg->payload()->data()));
    auto p2pMessage = std::dynamic_pointer_cast<P2PMessage>(msg);
    auto const& from = (p2pMessage->header()->srcGwNode().size() > 0) ?
                           p2pMessage->header()->srcGwNode() :
                           session->nodeId();
    auto statusSeqChanged = statusChanged(from, statusSeq);
    if (!statusSeqChanged)
    {
        return;
    }
    // status changed, request for the nodeStatus
    ROUTER_MGR_LOG(TRACE)
        << LOG_DESC("onReceiveNodeSeqMessage: statusChanged, request the lastest nodeStatus")
        << LOG_KV("node", printP2PIDElegantly(from)) << LOG_KV("statusSeq", statusSeq);
    m_service->asyncSendMessageByP2PNodeID(
        (uint16_t)GatewayPacketType::RequestNodeStatus, from, std::make_shared<bcos::bytes>());
}


bool GatewayRouterManager::statusChanged(std::string const& p2pNodeID, uint32_t seq)
{
    bool ret = true;
    ReadGuard l(x_p2pID2Seq);
    auto it = m_p2pID2Seq.find(p2pNodeID);
    if (it != m_p2pID2Seq.end())
    {
        ret = (seq > it->second);
    }
    return ret;
}

void GatewayRouterManager::broadcastStatusSeq()
{
    m_timer->restart();
    auto message =
        std::dynamic_pointer_cast<P2PMessage>(m_service->messageFactory()->buildMessage());
    message->setPacketType((uint16_t)GatewayPacketType::SyncNodeSeq);
    auto seq = m_localRouter->statusSeq();
    auto statusSeq = boost::asio::detail::socket_ops::host_to_network_long(seq);
    auto payload = std::make_shared<bytes>((byte*)&statusSeq, (byte*)&statusSeq + 4);
    message->setPayload(payload);
    ROUTER_MGR_LOG(TRACE) << LOG_DESC("broadcastStatusSeq") << LOG_KV("seq", seq);
    m_service->asyncBroadcastMessage(message);
}


void GatewayRouterManager::onReceiveRequestNodeStatusMsg(
    MessageFace::Ptr msg, WsSession::Ptr session)
{
    auto p2pMessage = std::dynamic_pointer_cast<P2PMessage>(msg);
    auto const& from = (!p2pMessage->header()->srcGwNode().empty()) ?
                           p2pMessage->header()->srcGwNode() :
                           session->nodeId();

    auto nodeStatusData = m_localRouter->generateNodeStatus();
    if (!nodeStatusData)
    {
        ROUTER_MGR_LOG(WARNING) << LOG_DESC(
                                       "onReceiveRequestNodeStatusMsg: generate nodeInfo error")
                                << LOG_KV("from", printP2PIDElegantly(from));
        return;
    }
    ROUTER_MGR_LOG(TRACE) << LOG_DESC(
                                 "onReceiveRequestNodeStatusMsg: response the latest nodeStatus")
                          << LOG_KV("node", printP2PIDElegantly(from));
    m_service->asyncSendMessageByP2PNodeID(
        (uint16_t)GatewayPacketType::ResponseNodeStatus, from, nodeStatusData);
}

void GatewayRouterManager::onRecvResponseNodeStatusMsg(MessageFace::Ptr msg, WsSession::Ptr session)
{
    auto nodeStatus = m_nodeStatusFactory->build();
    nodeStatus->decode(bytesConstRef(msg->payload()->data(), msg->payload()->size()));

    auto p2pMessage = std::dynamic_pointer_cast<P2PMessage>(msg);
    auto const& from = (!p2pMessage->header()->srcGwNode().empty()) ?
                           p2pMessage->header()->srcGwNode() :
                           session->nodeId();

    ROUTER_MGR_LOG(INFO) << LOG_DESC("onRecvResponseNodeStatusMsg")
                         << LOG_KV("node", printP2PIDElegantly(from))
                         << LOG_KV("statusSeq", nodeStatus->statusSeq())
                         << LOG_KV("agency", nodeStatus->agency());
    updatePeerNodeStatus(from, nodeStatus);
}

void GatewayRouterManager::updatePeerNodeStatus(
    std::string const& p2pID, GatewayNodeInfo::Ptr status)
{
    auto statusSeq = status->statusSeq();
    {
        UpgradableGuard l(x_p2pID2Seq);
        if (m_p2pID2Seq.contains(p2pID) && (m_p2pID2Seq.at(p2pID) >= statusSeq))
        {
            return;
        }
        UpgradeGuard ul(l);
        m_p2pID2Seq[p2pID] = statusSeq;
    }
    ROUTER_MGR_LOG(INFO) << LOG_DESC("updatePeerNodeStatus")
                         << LOG_KV("node", printP2PIDElegantly(p2pID))
                         << LOG_KV("statusSeq", status->statusSeq())
                         << LOG_KV("agency", status->agency());
    m_peerRouter->updateGatewayInfo(status);
}