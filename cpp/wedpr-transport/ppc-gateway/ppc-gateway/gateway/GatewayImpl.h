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
 * @file GatewayImpl.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-framework/gateway/IGateway.h"
#include "ppc-gateway/gateway/router/GatewayNodeInfo.h"
#include "ppc-gateway/p2p/Service.h"
#include "ppc-gateway/p2p/router/RouterManager.h"
#include "router/GatewayRouterManager.h"
#include "router/LocalRouter.h"
#include "router/PeerRouterTable.h"

namespace ppc::gateway
{
class GatewayImpl : public IGateway, public std::enable_shared_from_this<GatewayImpl>
{
public:
    using Ptr = std::shared_ptr<GatewayImpl>;
    GatewayImpl(Service::Ptr const& service, ppc::front::IFrontBuilder::Ptr const& frontBuilder,
        std::shared_ptr<boost::asio::io_service> ioService, std::string const& agency);
    ~GatewayImpl() override = default;

    void start() override;
    void stop() override;

    /**
     * @brief send message to gateway
     *
     * @param routeType the route type
     * @param topic  the topic
     * @param dstInst the dst agency(must set when 'route by agency' and 'route by
     * component')
     * @param dstNodeID  the dst nodeID(must set when 'route by nodeID')
     * @param componentType the componentType(must set when 'route by component')
     * @param payload the payload to send
     * @param seq the message seq
     * @param timeout timeout
     * @param callback callback
     */
    void asyncSendMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID,
        bcos::bytes&& payload, long timeout, ppc::protocol::ReceiveMsgFunc callback) override;

    void asyncSendbroadcastMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID,
        bcos::bytes&& payload) override;


    bcos::Error::Ptr registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) override;
    bcos::Error::Ptr unRegisterNodeInfo(bcos::bytesConstRef nodeID) override;
    bcos::Error::Ptr registerTopic(bcos::bytesConstRef nodeID, std::string const& topic) override;
    bcos::Error::Ptr unRegisterTopic(bcos::bytesConstRef nodeID, std::string const& topic) override;

    void asyncGetPeers(std::function<void(bcos::Error::Ptr, std::string)> callback) override;
    void asyncGetAgencies(std::vector<std::string> const& components,
        std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback) override;

    std::vector<std::string> selectNodesByRoutePolicy(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) override
    {
        return m_peerRouter->selectTargetNodes(routeType, routeInfo);
    }

protected:
    virtual void onReceiveP2PMessage(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);
    virtual void onReceiveBroadcastMessage(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);

private:
    bool m_running = false;
    Service::Ptr m_service;
    ppc::protocol::P2PMessageBuilder::Ptr m_msgBuilder;

    ppc::front::IFrontBuilder::Ptr m_frontBuilder;
    std::string m_agency;

    RouterManager::Ptr m_p2pRouterManager;
    GatewayRouterManager::Ptr m_gatewayRouterManager;

    GatewayNodeInfoFactory::Ptr m_gatewayInfoFactory;
    LocalRouter::Ptr m_localRouter;
    PeerRouterTable::Ptr m_peerRouter;
};
}  // namespace ppc::gateway