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
 * @file GatewayClient.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "GrpcClient.h"
#include "ppc-framework/gateway/IGateway.h"

namespace ppc::protocol
{
class GatewayClient : public ppc::gateway::IGateway, public GrpcClient
{
public:
    using Ptr = std::shared_ptr<GatewayClient>;
    GatewayClient(ppc::protocol::GrpcConfig::Ptr const& grpcConfig, std::string const& endPoints,
        INodeInfoFactory::Ptr nodeInfoFactory);

    ~GatewayClient() override = default;


    void start() override {}
    void stop() override {}

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

    void asyncGetPeers(std::function<void(bcos::Error::Ptr, std::string)> callback) override;
    void asyncGetAgencies(std::vector<std::string> const& components,
        std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback) override;

    void asyncSendbroadcastMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID,
        bcos::bytes&& payload) override
    {}
    bcos::Error::Ptr registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) override;
    bcos::Error::Ptr unRegisterNodeInfo(bcos::bytesConstRef nodeID) override;
    bcos::Error::Ptr registerTopic(bcos::bytesConstRef nodeID, std::string const& topic) override;
    bcos::Error::Ptr unRegisterTopic(bcos::bytesConstRef nodeID, std::string const& topic) override;

    std::vector<std::string> selectNodesByRoutePolicy(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) override;

    std::vector<ppc::protocol::INodeInfo::Ptr> getAliveNodeList() const override;

private:
    std::unique_ptr<ppc::proto::Gateway::Stub> m_stub;
    std::map<std::string, std::unique_ptr<ppc::proto::Gateway::Stub>> m_broadcastStubs;
    INodeInfoFactory::Ptr m_nodeInfoFactory;
};
}  // namespace ppc::protocol