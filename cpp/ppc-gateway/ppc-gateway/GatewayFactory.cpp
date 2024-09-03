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
 * @file GatewayFactory.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */
#include "GatewayFactory.h"
#include "Common.h"
#include "bcos-boostssl/websocket/WsInitializer.h"
#include "ppc-gateway/p2p/Service.h"
#include "ppc-gateway/p2p/router/RouterTableImpl.h"
#include "protocol/src/v1/MessageHeaderImpl.h"
#include "protocol/src/v1/MessageImpl.h"

using namespace ppc;
using namespace bcos;
using namespace ppc::tools;
using namespace ppc::protocol;
using namespace ppc::gateway;
using namespace bcos::boostssl::ws;
using namespace bcos::boostssl;

WsConfig::Ptr GatewayFactory::createServiceConfig(GatewayConfig const& config) const
{
    auto wsConfig = std::make_shared<WsConfig>();
    wsConfig->setModel(WsModel::Mixed);
    wsConfig->setListenIP(config.networkConfig.listenIp);
    wsConfig->setListenPort(config.networkConfig.listenPort);
    wsConfig->setSmSSL(config.networkConfig.enableSM);
    wsConfig->setMaxMsgSize(config.maxAllowedMsgSize);
    wsConfig->setReconnectPeriod(config.reconnectTime);
    // TODO: setHeartbeatPeriod, setSendMsgTimeout
    wsConfig->setThreadPoolSize(config.networkConfig.threadPoolSize);
    // connected peers
    wsConfig->setConnectPeers(m_gatewayConfig->nodeIPEndpointSetPtr());
    wsConfig->setDisableSsl(config.networkConfig.disableSsl);
    wsConfig->setContextConfig(m_contextConfig->contextConfig());
    return wsConfig;
}

Service::Ptr GatewayFactory::buildService() const
{
    auto wsConfig = createServiceConfig(m_config->gatewayConfig());
    auto wsInitializer = std::make_shared<WsInitializer>();
    // set the messageFactory
    wsInitializer->setMessageFactory(
        std::make_shared<MessageBuilderImpl>(std::make_shared<MessageHeaderBuilderImpl>()));
    // set the config
    wsInitializer->setConfig(wsConfig);
    auto p2pService = std::make_shared<Service>(m_contextConfig->nodeID(),
        std::make_shared<RouterTableFactoryImpl>(), m_config->gatewayConfig().unreachableDistance,
        "Service");
    p2pService->setNodeEndpoints(m_gatewayConfig->nodeIPEndpointSet());

    wsInitializer->initWsService(p2pService);
    return p2pService;
}

IGateway::Ptr GatewayFactory::build(ppc::front::IFrontBuilder::Ptr const& frontBuilder) const
{
    auto service = buildService();
    return std::make_shared<GatewayImpl>(
        service, frontBuilder, std::make_shared<boost::asio::io_service>(), m_config->agencyID());
}