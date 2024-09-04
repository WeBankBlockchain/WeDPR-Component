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
 * @file ProTransportImpl.cpp
 * @author: yujiechen
 * @date 2024-09-04
 */
#include "ProTransportImpl.h"
#include "protocol/src/v1/MessageImpl.h"
#include "wedpr-protocol/grpc/server/FrontServer.h"

using namespace ppc::front;
using namespace ppc::protocol;
using namespace ppc::sdk;


ProTransportImpl::ProTransportImpl(ppc::front::FrontConfig::Ptr config)
  : m_config(std::move(config))
{
    GrpcServerConfig grpcServerConfig{config->selfEndPoint()};
    m_server = std::make_shared<GrpcServer>(grpcServerConfig);

    FrontFactory frontFactory;
    grpc::ChannelArguments channelConfig;
    channelConfig.SetLoadBalancingPolicyName(m_config->loadBalancePolicy());
    auto gateway = std::make_shared<GatewayClient>(channelConfig, m_config->gatewayEndPoints());
    m_front = frontFactory.build(std::make_shared<NodeInfoFactory>(),
        std::make_shared<MessagePayloadBuilderImpl>(),
        std::make_shared<MessageOptionalHeaderBuilderImpl>(), gateway, config);

    auto msgBuilder =
        std::make_shared<MessageBuilderImpl>(std::make_shared<MessageHeaderBuilderImpl>());
    auto frontService = std::make_shared<FrontServer>(msgBuilder, m_front);

    // register the frontService
    m_server->registerService(frontService);
}