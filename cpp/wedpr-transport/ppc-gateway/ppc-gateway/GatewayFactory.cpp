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
#include "bcos-boostssl/utility/NewTimer.h"
#include "bcos-boostssl/websocket/WsInitializer.h"
#include "ppc-gateway/p2p/Service.h"
#include "ppc-gateway/p2p/router/RouterTableImpl.h"
#include "ppc-gateway/protocol/P2PMessageImpl.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "protocol/src/v1/MessageHeaderImpl.h"
#include "protocol/src/v1/MessageImpl.h"

using namespace ppc;
using namespace bcos;
using namespace ppc::tools;
using namespace ppc::protocol;
using namespace ppc::gateway;
using namespace bcos::boostssl::ws;
using namespace bcos::boostssl;

Service::Ptr GatewayFactory::buildService() const
{
    auto gwConfig = m_config->gatewayConfig();
    auto wsConfig = std::make_shared<WsConfig>();
    wsConfig->setModel(WsModel::Mixed);
    wsConfig->setListenIP(gwConfig.networkConfig.listenIp);
    wsConfig->setListenPort(gwConfig.networkConfig.listenPort);
    wsConfig->setSmSSL(gwConfig.networkConfig.enableSM);
    wsConfig->setMaxMsgSize(gwConfig.maxAllowedMsgSize);
    wsConfig->setReconnectPeriod(gwConfig.reconnectTime);
    // TODO: setHeartbeatPeriod, setSendMsgTimeout
    wsConfig->setThreadPoolSize(gwConfig.networkConfig.threadPoolSize);
    // connected peers
    wsConfig->setConnectPeers(m_gatewayConfig->nodeIPEndpointSetPtr());
    wsConfig->setDisableSsl(gwConfig.networkConfig.disableSsl);
    wsConfig->setContextConfig(m_contextConfig->contextConfig());

    auto wsInitializer = std::make_shared<WsInitializer>();
    // set the messageFactory
    auto msgBuilder =
        std::make_shared<MessageBuilderImpl>(std::make_shared<MessageHeaderBuilderImpl>());
    wsInitializer->setMessageFactory(std::make_shared<P2PMessageBuilderImpl>(msgBuilder));
    // set the config
    wsInitializer->setConfig(wsConfig);
    auto p2pService = std::make_shared<Service>(m_contextConfig->nodeID(),
        std::make_shared<RouterTableFactoryImpl>(), m_config->gatewayConfig().unreachableDistance,
        "Gateway-Service");
    p2pService->setTimerFactory(std::make_shared<bcos::timer::TimerFactory>());
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