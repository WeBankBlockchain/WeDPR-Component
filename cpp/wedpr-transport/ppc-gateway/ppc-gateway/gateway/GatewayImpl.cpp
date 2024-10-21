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
 * @file GatewayImpl.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */
#include "GatewayImpl.h"
#include "SendMessageWithRetry.h"
#include "cache/MessageCache.h"
#include "ppc-framework/gateway/GatewayProtocol.h"
#include "router/GatewayNodeInfoImpl.h"

using namespace bcos;
using namespace ppc;
using namespace ppc::protocol;
using namespace ppc::gateway;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;

GatewayImpl::GatewayImpl(Service::Ptr const& service,
    ppc::front::IFrontBuilder::Ptr const& frontBuilder,
    std::shared_ptr<boost::asio::io_service> ioService, std::string const& agency)
  : m_service(service),
    m_msgBuilder(
        std::dynamic_pointer_cast<ppc::protocol::MessageBuilder>(service->messageFactory())),
    m_frontBuilder(frontBuilder),
    m_agency(agency),
    m_p2pRouterManager(std::make_shared<RouterManager>(service)),
    m_gatewayInfoFactory(std::make_shared<GatewayNodeInfoFactoryImpl>(service->nodeID(), agency)),
    m_localRouter(std::make_shared<LocalRouter>(
        m_gatewayInfoFactory, m_frontBuilder, std::make_shared<MessageCache>(ioService))),
    m_peerRouter(std::make_shared<PeerRouterTable>(m_service, m_gatewayInfoFactory))
{
    m_service->registerMsgHandler((uint16_t)GatewayPacketType::P2PMessage,
        boost::bind(&GatewayImpl::onReceiveP2PMessage, this, boost::placeholders::_1,
            boost::placeholders::_2));

    m_service->registerMsgHandler((uint16_t)GatewayPacketType::BroadcastMessage,
        boost::bind(&GatewayImpl::onReceiveBroadcastMessage, this, boost::placeholders::_1,
            boost::placeholders::_2));
    m_gatewayRouterManager = std::make_shared<GatewayRouterManager>(
        m_service, m_gatewayInfoFactory, m_localRouter, m_peerRouter);

    m_service->registerOnNewSession([this](WsSession::Ptr _session) {
        if (!_session)
        {
            return;
        }
        m_p2pRouterManager->onNewSession(_session->nodeId());
    });

    m_service->registerOnDeleteSession([this](WsSession::Ptr _session) {
        if (!_session)
        {
            return;
        }
        m_p2pRouterManager->onEraseSession(_session->nodeId());
    });

    m_p2pRouterManager->registerUnreachableHandler([this](std::string const& unreachableNode) {
        m_gatewayRouterManager->removeUnreachableP2pNode(unreachableNode);
    });

    m_service->registerDisconnectHandler([this](WsSession::Ptr _session) {
        if (!_session)
        {
            return;
        }
        m_gatewayRouterManager->removeUnreachableP2pNode(_session->nodeId());
    });
}

void GatewayImpl::start()
{
    if (m_running)
    {
        GATEWAY_LOG(INFO) << LOG_DESC("Gateway has already been started");
        return;
    }
    m_running = true;
    if (m_service)
    {
        m_service->start();
    }
    if (m_p2pRouterManager)
    {
        m_p2pRouterManager->start();
    }
    if (m_gatewayRouterManager)
    {
        m_gatewayRouterManager->start();
    }
    GATEWAY_LOG(INFO) << LOG_DESC("Start gateway success");
}

void GatewayImpl::stop()
{
    if (!m_running)
    {
        GATEWAY_LOG(INFO) << LOG_DESC("Gateway has already been stopped");
        return;
    }
    m_running = false;
    if (m_service)
    {
        m_service->stop();
    }
    if (m_p2pRouterManager)
    {
        m_p2pRouterManager->stop();
    }
    if (m_gatewayRouterManager)
    {
        m_gatewayRouterManager->stop();
    }
    PEER_ROUTER_LOG(INFO) << LOG_DESC("Stop gateway success");
}

void GatewayImpl::asyncSendbroadcastMessage(ppc::protocol::RouteType routeType,
    MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID, bcos::bytes&& payload)
{
    // dispatcher to all the local front
    routeInfo->setDstNode(bcos::bytes());
    routeInfo->setSrcInst(m_agency);

    auto p2pMessage = m_msgBuilder->build(routeType, routeInfo, std::move(payload));
    p2pMessage->setSeq(traceID);

    p2pMessage->setPacketType((uint16_t)GatewayPacketType::BroadcastMessage);
    m_localRouter->dispatcherMessage(p2pMessage, nullptr);
    // broadcast message to all peers
    m_peerRouter->asyncBroadcastMessage(p2pMessage);
}


void GatewayImpl::asyncSendMessage(ppc::protocol::RouteType routeType,
    ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID,
    bcos::bytes&& payload, long timeout, ReceiveMsgFunc callback)
{
    routeInfo->setSrcInst(m_agency);
    // check the localRouter
    auto p2pMessage = m_msgBuilder->build(routeType, routeInfo, std::move(payload));
    p2pMessage->setSeq(traceID);
    p2pMessage->setPacketType((uint16_t)GatewayPacketType::P2PMessage);
    GATEWAY_LOG(TRACE) << LOG_DESC("asyncSendMessage") << LOG_KV("msg", printMessage(p2pMessage));
    auto nodeList = m_localRouter->chooseReceiver(p2pMessage);
    // case send to the same agency
    if (!nodeList.empty())
    {
        GATEWAY_LOG(TRACE) << LOG_DESC("hit the local router, dispatch message directly")
                           << LOG_KV("msg", printMessage(p2pMessage));
        m_localRouter->dispatcherMessage(p2pMessage, callback);
        return;
    }
    // try to find the dstP2PNode
    auto selectedP2PNodes =
        m_peerRouter->selectRouter(routeType, p2pMessage->header()->optionalField());
    if (selectedP2PNodes.empty())
    {
        GATEWAY_LOG(INFO) << LOG_DESC("can't find the gateway to send the message")
                          << LOG_KV("detail", printMessage(p2pMessage));
        if (callback)
        {
            callback(std::make_shared<bcos::Error>(
                -1, "can't find the gateway to send the message, traceID: " +
                        p2pMessage->header()->traceID()));
        }
        return;
    }
    // send the message to gateway
    auto retry = std::make_shared<SendMessageWithRetry>(
        m_service, std::move(selectedP2PNodes), std::move(p2pMessage), callback, timeout);
    retry->trySendMessage();
}

void GatewayImpl::onReceiveP2PMessage(MessageFace::Ptr msg, WsSession::Ptr session)
{
    // try to dispatcher to the front
    auto p2pMessage = std::dynamic_pointer_cast<Message>(msg);
    auto self = std::weak_ptr<GatewayImpl>(shared_from_this());
    // Note: the callback can only been called once since it binds the callback seq
    auto callback = [p2pMessage, session, self](Error::Ptr error) {
        auto gateway = self.lock();
        if (!gateway)
        {
            return;
        }
        // Note: no need to sendResponse for the response packet
        if (p2pMessage->isRespPacket())
        {
            return;
        }
        std::string errorCode = std::to_string(CommonError::SUCCESS);
        if (error && error->errorCode() != 0)
        {
            GATEWAY_LOG(WARNING) << LOG_DESC("onReceiveP2PMessage: dispatcherMessage failed")
                                 << LOG_KV("code", error->errorCode())
                                 << LOG_KV("msg", error->errorMessage())
                                 << printMessage(p2pMessage);
            errorCode = std::to_string(error->errorCode());
        }

        std::shared_ptr<bcos::bytes> payload =
            std::make_shared<bcos::bytes>(errorCode.begin(), errorCode.end());
        gateway->m_service->sendRespMessageBySession(session, p2pMessage, std::move(payload));
    };

    auto ret = m_localRouter->dispatcherMessage(p2pMessage, callback);
    if (!ret)
    {
        GATEWAY_LOG(ERROR)
            << LOG_DESC(
                   "onReceiveP2PMessage failed to find the node that can dispatch this message")
            << LOG_KV("msg", printMessage(p2pMessage));
        callback(std::make_shared<bcos::Error>(CommonError::NotFoundFrontServiceDispatchMsg,
            "unable to find the node to dispatcher this message, message detail: " +
                printMessage(p2pMessage)));
    }
}

void GatewayImpl::onReceiveBroadcastMessage(MessageFace::Ptr msg, WsSession::Ptr)
{
    auto p2pMessage = std::dynamic_pointer_cast<Message>(msg);
    GATEWAY_LOG(TRACE) << LOG_DESC("onReceiveBroadcastMessage, dispatcher")
                       << LOG_KV("msg", printMessage(p2pMessage));
    m_localRouter->dispatcherMessage(p2pMessage, nullptr);
}

bcos::Error::Ptr GatewayImpl::registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo)
{
    auto self = weak_from_this();
    m_localRouter->registerNodeInfo(
        nodeInfo,
        [nodeInfo, self]() {
            // remove the unhealth node
            GATEWAY_LOG(INFO) << LOG_DESC("Remove the unhealth node") << printNodeInfo(nodeInfo);
            auto gateway = self.lock();
            if (!gateway)
            {
                return;
            }
            gateway->m_localRouter->unRegisterNode(nodeInfo->nodeID().toBytes());
            gateway->m_localRouter->increaseSeq();
        },
        true);
    return nullptr;
}

bcos::Error::Ptr GatewayImpl::unRegisterNodeInfo(bcos::bytesConstRef nodeID)
{
    m_localRouter->unRegisterNode(nodeID.toBytes());
    return nullptr;
}

bcos::Error::Ptr GatewayImpl::registerTopic(bcos::bytesConstRef nodeID, std::string const& topic)
{
    m_localRouter->registerTopic(nodeID, topic);
    return nullptr;
}

bcos::Error::Ptr GatewayImpl::unRegisterTopic(bcos::bytesConstRef nodeID, std::string const& topic)
{
    m_localRouter->unRegisterTopic(nodeID, topic);
    return nullptr;
}

void GatewayImpl::asyncGetPeers(std::function<void(Error::Ptr, std::string)> callback)
{
    if (!callback)
    {
        return;
    }
    try
    {
        auto infos = m_peerRouter->gatewayInfos();
        Json::Value peers;
        peers["agency"] = m_agency;
        peers["nodeID"] = m_service->nodeID();
        // add the local gatewayInfo
        Json::Value localGatewayInfo;
        m_localRouter->routerInfo()->toJson(localGatewayInfo);
        peers["gateway"] = localGatewayInfo;
        peers["peers"] = Json::Value(Json::arrayValue);
        for (auto const& it : infos)
        {
            auto gatewayInfoList = it.second;
            Json::Value agencyGatewayInfo;
            agencyGatewayInfo["agency"] = it.first;
            Json::Value peersInfo(Json::arrayValue);
            for (auto const& gatewayInfo : gatewayInfoList)
            {
                Json::Value gatewayJson;
                gatewayInfo->toJson(gatewayJson);
                peersInfo.append(gatewayJson);
            }
            agencyGatewayInfo["gateway"] = peersInfo;
            peers["peers"].append(agencyGatewayInfo);
        }
        Json::FastWriter fastWriter;
        std::string statusStr = fastWriter.write(peers);
        callback(nullptr, statusStr);
    }
    catch (std::exception const& e)
    {
        GATEWAY_LOG(WARNING) << LOG_DESC("asyncGetPeers exception")
                             << LOG_KV("error", boost::diagnostic_information(e));
        callback(
            std::make_shared<bcos::Error>(
                -1, "asyncGetPeers exception for " + std::string(boost::diagnostic_information(e))),
            "");
    }
}

void GatewayImpl::asyncGetAgencies(std::vector<std::string> const& components,
    std::function<void(Error::Ptr, std::set<std::string>)> callback)
{
    if (!callback)
    {
        return;
    }
    auto agencies = m_peerRouter->agencies(components);
    agencies.insert(m_agency);
    callback(nullptr, agencies);
}