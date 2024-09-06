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
 * @file Service.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */

#include "Service.h"
#include "bcos-boostssl/websocket/WsError.h"
#include "ppc-framework/Common.h"

using namespace bcos;
using namespace ppc;
using namespace ppc::gateway;
using namespace ppc::protocol;
using namespace bcos::boostssl::ws;
using namespace bcos::boostssl;

Service::Service(std::string const& _nodeID, RouterTableFactory::Ptr const& _routerTableFactory,
    int unreachableDistance, std::string _moduleName)
  : WsService(_moduleName)
{
    m_nodeID = _nodeID;
    m_routerTableFactory = _routerTableFactory;
    // create the local router
    m_routerTable = m_routerTableFactory->createRouterTable();
    m_routerTable->setNodeID(m_nodeID);
    m_routerTable->setUnreachableDistance(unreachableDistance);

    SERVICE_LOG(INFO) << LOG_DESC("create P2PService") << LOG_KV("module", _moduleName);
    WsService::registerConnectHandler(
        boost::bind(&Service::onP2PConnect, this, boost::placeholders::_1));
    WsService::registerDisconnectHandler(
        boost::bind(&Service::onP2PDisconnect, this, boost::placeholders::_1));
}


void Service::onP2PConnect(WsSession::Ptr _session)
{
    SERVICE_LOG(INFO) << LOG_DESC("Receive new p2p connection")
                      << LOG_KV("p2pid", printP2PIDElegantly(_session->nodeId()))
                      << LOG_KV("endpoint", _session->endPoint());


    RecursiveGuard l(x_nodeID2Session);
    auto it = m_nodeID2Session.find(_session->nodeId());
    // the session already connected
    if (it != m_nodeID2Session.end() && it->second->isConnected())
    {
        SERVICE_LOG(INFO) << LOG_DESC("onP2PConnect, drop the duplicated connection")
                          << LOG_KV("nodeID", printP2PIDElegantly(_session->nodeId()))
                          << LOG_KV("endpoint", _session->endPoint());
        _session->drop(WsError::UserDisconnect);
        updateNodeIDInfo(_session);
        return;
    }
    // the node-self
    if (_session->nodeId() == m_nodeID)
    {
        updateNodeIDInfo(_session);
        SERVICE_LOG(INFO) << LOG_DESC("onP2PConnect, drop the node-self connection")
                          << LOG_KV("nodeID", printP2PIDElegantly(_session->nodeId()))
                          << LOG_KV("endpoint", _session->endPoint());
        _session->drop(WsError::UserDisconnect);
        return;
    }


    ///// Note: here allow all new session, even the ip not configured(support dynamic access)
    bool updated = updateNodeIDInfo(_session);
    // hit the m_nodeID2Session
    if (it != m_nodeID2Session.end())
    {
        // the old session has already been connected, and the new session endPoint is not
        // configured
        if (it->second->isConnected() && !updated)
        {
            SERVICE_LOG(INFO) << LOG_DESC(
                                     "onP2PConnect, drop the new not-configurated session, remain "
                                     "the old session")
                              << LOG_KV("nodeID", printP2PIDElegantly(_session->nodeId()))
                              << LOG_KV("endPoint", _session->endPoint())
                              << LOG_KV("oldEndPoint", it->second->endPoint());
            _session->drop(WsError::UserDisconnect);
            return;
        }
        SERVICE_LOG(INFO) << LOG_DESC(
                                 "onP2PConnect, drop the old not-configurated session, replace "
                                 "with the new session")
                          << LOG_KV("nodeID", printP2PIDElegantly(_session->nodeId()))
                          << LOG_KV("endPoint", _session->endPoint())
                          << LOG_KV("oldEndPoint", it->second->endPoint());
        if (it->second->isConnected())
        {
            it->second->drop(WsError::UserDisconnect);
        }
        it->second = _session;
        return;
    }
    // the new session
    m_nodeID2Session.insert(std::make_pair(_session->nodeId(), _session));
    SERVICE_LOG(INFO) << LOG_DESC("onP2PConnect established new session")
                      << LOG_KV("p2pid", printP2PIDElegantly(_session->nodeId()))
                      << LOG_KV("endpoint", _session->endPoint());
}


bool Service::updateNodeIDInfo(WsSession::Ptr const& _session)
{
    bcos::WriteGuard l(x_configuredNode2ID);
    std::string p2pNodeID = _session->nodeId();
    auto it = m_configuredNode2ID.find(_session->endPointInfo());
    if (it != m_configuredNode2ID.end())
    {
        it->second = p2pNodeID;
        SERVICE_LOG(INFO) << LOG_DESC("updateNodeIDInfo: update the nodeID")
                          << LOG_KV("nodeid", p2pNodeID)
                          << LOG_KV("endpoint", _session->endPoint());
        return true;
    }

    SERVICE_LOG(INFO) << LOG_DESC("updateNodeIDInfo can't find endpoint")
                      << LOG_KV("nodeid", printP2PIDElegantly(p2pNodeID))
                      << LOG_KV("endpoint", _session->endPoint());
    return false;
}

void Service::removeSessionInfo(WsSession::Ptr const& _session)
{
    RecursiveGuard l(x_nodeID2Session);
    auto it = m_nodeID2Session.find(_session->nodeId());
    if (it != m_nodeID2Session.end())
    {
        SERVICE_LOG(INFO) << "onP2PDisconnectand remove from m_nodeID2Session"
                          << LOG_KV("p2pid", printP2PIDElegantly(_session->nodeId()))
                          << LOG_KV("endpoint", _session->endPoint());

        m_nodeID2Session.erase(it);
    }
}
void Service::onP2PDisconnect(WsSession::Ptr _session)
{
    // remove the session information
    removeSessionInfo(_session);
    // update the session nodeID to empty
    UpgradableGuard l(x_configuredNode2ID);
    for (auto& it : m_configuredNode2ID)
    {
        // reset the nodeID of the dropped session(except the node-self) to empty
        if (m_nodeID != _session->nodeId() && it.second == _session->nodeId())
        {
            UpgradeGuard ul(l);
            it.second.clear();
            break;
        }
    }
}

void Service::reconnect()
{
    // obtain the un-connected peers information
    EndPointsPtr unconnectedPeers = std::make_shared<std::set<NodeIPEndpoint>>();
    {
        bcos::ReadGuard l(x_configuredNode2ID);
        for (auto const& it : m_configuredNode2ID)
        {
            if (it.second == nodeID())
            {
                continue;
            }
            if (!it.second.empty() && isConnected(it.first))
            {
                continue;
            }
            unconnectedPeers->insert(it.first);
            SERVICE_LOG(DEBUG) << LOG_DESC("ready to reconnect")
                               << LOG_KV("endpoint",
                                      it.first.address() + ":" + std::to_string(it.first.port()));
        }
    }
    setReconnectedPeers(unconnectedPeers);
    WsService::reconnect();
}

WsSession::Ptr Service::getSessionByNodeID(std::string const& _nodeID)
{
    RecursiveGuard l(x_nodeID2Session);
    auto it = m_nodeID2Session.find(_nodeID);
    if (it == m_nodeID2Session.end())
    {
        return nullptr;
    }
    return it->second;
}

void Service::asyncSendMessageByNodeID(
    std::string const& dstNodeID, MessageFace::Ptr msg, Options options, RespCallBack respFunc)
{
    auto p2pMsg = std::dynamic_pointer_cast<Message>(msg);
    if (p2pMsg->header()->dstGwNode().empty())
    {
        p2pMsg->header()->setDstGwNode(dstNodeID);
    }
    if (p2pMsg->header()->srcGwNode().empty())
    {
        p2pMsg->header()->setSrcGwNode(m_nodeID);
    }
    return asyncSendMessageWithForward(dstNodeID, msg, options, respFunc);
}

void Service::asyncSendMessageWithForward(
    std::string const& dstNodeID, MessageFace::Ptr msg, Options options, RespCallBack respFunc)
{
    auto p2pMsg = std::dynamic_pointer_cast<Message>(msg);
    // without nextHop: maybe network unreachable or with distance equal to 1
    auto nextHop = m_routerTable->getNextHop(dstNodeID);
    if (nextHop.empty())
    {
        return asyncSendMessage(dstNodeID, msg, options, respFunc);
    }
    // with nextHop, send the message to nextHop
    SERVICE_LOG(TRACE) << LOG_DESC("asyncSendMessageByNodeID") << printMessage(p2pMsg);
    return asyncSendMessage(nextHop, msg, options, respFunc);
}


void Service::asyncSendMessage(
    std::string const& dstNodeID, MessageFace::Ptr msg, Options options, RespCallBack respFunc)
{
    try
    {
        // ignore self
        if (dstNodeID == m_nodeID)
        {
            return;
        }
        if (msg->seq().empty())
        {
            msg->setSeq(m_messageFactory->newSeq());
        }
        auto session = getSessionByNodeID(dstNodeID);
        if (session)
        {
            WsSessions sessions = WsSessions();
            sessions.emplace_back(session);
            return WsService::asyncSendMessage(sessions, msg, options, respFunc);
        }

        if (respFunc)
        {
            Error::Ptr error = std::make_shared<Error>(
                -1, "send message to " + dstNodeID +
                        " failed for no network established, msg: " + printWsMessage(msg));
            respFunc(std::move(error), nullptr, nullptr);
        }
        SERVICE_LOG(WARNING)
            << LOG_DESC("asyncSendMessageByNodeID failed for no network established, msg detail:")
            << printWsMessage(msg);
    }
    catch (std::exception const& e)
    {
        SERVICE_LOG(ERROR) << "asyncSendMessageByNodeID" << LOG_KV("dstNode", dstNodeID)
                           << LOG_KV("what", boost::diagnostic_information(e));
        if (respFunc)
        {
            respFunc(std::make_shared<Error>(-1, "send message to " + dstNodeID + " failed for " +
                                                     boost::diagnostic_information(e)),
                nullptr, nullptr);
        }
    }
}

void Service::onRecvMessage(MessageFace::Ptr _msg, std::shared_ptr<WsSession> _session)
{
    auto p2pMsg = std::dynamic_pointer_cast<Message>(_msg);
    // find the dstNode
    if (p2pMsg->header()->dstGwNode().empty() || p2pMsg->header()->dstGwNode() == m_nodeID)
    {
        SERVICE_LOG(TRACE) << LOG_DESC("onRecvMessage, dispatch for find the dst node")
                           << printMessage(p2pMsg);
        WsService::onRecvMessage(_msg, _session);
        return;
    }
    // forward the message
    if (p2pMsg->header()->ttl() >= m_routerTable->unreachableDistance())
    {
        SERVICE_LOG(WARNING) << LOG_DESC("onRecvMessage: ttl expired") << printMessage(p2pMsg);
        return;
    }
    p2pMsg->header()->setTTL(p2pMsg->header()->ttl() + 1);
    asyncSendMessageWithForward(
        p2pMsg->header()->dstGwNode(), p2pMsg, bcos::boostssl::ws::Options(), nullptr);
}


void Service::asyncBroadcastMessage(bcos::boostssl::MessageFace::Ptr msg, Options options)
{
    auto reachableNodes = m_routerTable->getAllReachableNode();
    try
    {
        for (auto const& node : reachableNodes)
        {
            asyncSendMessageByNodeID(node, msg, options);
        }
    }
    catch (std::exception& e)
    {
        SERVICE_LOG(WARNING) << LOG_BADGE("asyncBroadcastMessage exception")
                             << LOG_KV("msg", printWsMessage(msg))
                             << LOG_KV("error", boost::diagnostic_information(e));
    }
}

void Service::asyncSendMessageByP2PNodeID(uint16_t type, std::string const& dstNodeID,
    std::shared_ptr<bcos::bytes> payload, Options options, RespCallBack callback)
{
    auto message = m_messageFactory->buildMessage();
    message->setPacketType(type);
    message->setPayload(payload);
    asyncSendMessageByNodeID(dstNodeID, message, options, callback);
}

void Service::sendRespMessageBySession(bcos::boostssl::ws::WsSession::Ptr const& session,
    bcos::boostssl::MessageFace::Ptr msg, std::shared_ptr<bcos::bytes>&& payload)
{
    auto respMessage = std::dynamic_pointer_cast<Message>(m_messageFactory->buildMessage());
    auto requestMsg = std::dynamic_pointer_cast<Message>(msg);
    if (requestMsg->header() && requestMsg->header()->optionalField())
    {
        respMessage->header()->optionalField()->setDstNode(
            requestMsg->header()->optionalField()->srcNode());
        respMessage->header()->optionalField()->setSrcNode(
            requestMsg->header()->optionalField()->dstNode());
    }
    respMessage->header()->setTraceID(requestMsg->header()->traceID());
    respMessage->header()->setRespPacket();
    respMessage->header()->setRouteType(ppc::protocol::RouteType::ROUTE_THROUGH_NODEID);
    respMessage->setPayload(std::move(payload));

    WsSessions sessions;
    sessions.emplace_back(session);
    WsService::asyncSendMessage(sessions, respMessage);
    SERVICE_LOG(TRACE) << "sendRespMessageBySession" << LOG_KV("resp", printMessage(respMessage))
                       << LOG_KV("payload size", payload->size());
}