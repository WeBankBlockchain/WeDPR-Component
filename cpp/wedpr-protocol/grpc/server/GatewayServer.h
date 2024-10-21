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
 * @file GatewayServer.h
 * @author: yujiechen
 * @date 2024-09-03
 */
#pragma once
#include "Service.grpc.pb.h"
#include "ppc-framework/gateway/IGateway.h"
#include <memory>

namespace ppc::protocol
{
class GatewayServer : public ppc::proto::Gateway::CallbackService
{
public:
    using Ptr = std::shared_ptr<GatewayServer>;
    GatewayServer(ppc::gateway::IGateway::Ptr gateway,
        MessageOptionalHeaderBuilder::Ptr routeInfoBuilder, INodeInfoFactory::Ptr nodeInfoFactory)
      : m_gateway(std::move(gateway)),
        m_routeInfoBuilder(std::move(routeInfoBuilder)),
        m_nodeInfoFactory(std::move(nodeInfoFactory))
    {}
    virtual ~GatewayServer() = default;

    grpc::ServerUnaryReactor* asyncSendMessage(grpc::CallbackServerContext* context,
        const ppc::proto::SendedMessageRequest* sendedMsg, ppc::proto::Error* reply) override;

    grpc::ServerUnaryReactor* selectNodesByRoutePolicy(grpc::CallbackServerContext* context,
        const ppc::proto::SelectRouteRequest* selectRouteRequest,
        ppc::proto::NodeList* reply) override;

    grpc::ServerUnaryReactor* asyncGetPeers(grpc::CallbackServerContext* context,
        const ppc::proto::Empty* request, ppc::proto::PeersInfo* reply) override;
    grpc::ServerUnaryReactor* asyncGetAgencies(grpc::CallbackServerContext* context,
        const ppc::proto::Condition* request, ppc::proto::AgenciesInfo* reply) override;


    grpc::ServerUnaryReactor* registerNodeInfo(grpc::CallbackServerContext* context,
        const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply) override;

    grpc::ServerUnaryReactor* unRegisterNodeInfo(grpc::CallbackServerContext* context,
        const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply) override;

    grpc::ServerUnaryReactor* registerTopic(grpc::CallbackServerContext* context,
        const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply) override;

    grpc::ServerUnaryReactor* unRegisterTopic(grpc::CallbackServerContext* context,
        const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply) override;


private:
    ppc::gateway::IGateway::Ptr m_gateway;
    MessageOptionalHeaderBuilder::Ptr m_routeInfoBuilder;
    INodeInfoFactory::Ptr m_nodeInfoFactory;
};
};  // namespace ppc::protocol