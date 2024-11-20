/**
 *  Copyright (C) 2022 WeDPR.
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
 * @file GatewayInitializer.cpp
 * @author: yujiechen
 * @date 2022-11-14
 */
#include "GatewayInitializer.h"
#include "grpc/server/GatewayServer.h"
#include "grpc/server/GrpcServer.h"
#include "ppc-gateway/GatewayFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "protobuf/src/NodeInfoImpl.h"
#include "wedpr-protocol/grpc/client/HealthCheckTimer.h"
#include "wedpr-protocol/grpc/client/RemoteFrontBuilder.h"
#include "wedpr-protocol/protocol/src/v1/MessageHeaderImpl.h"

using namespace ppc::tools;
using namespace ppc::protocol;
using namespace bcos;
using namespace ppc::gateway;
using namespace ppc::front;

void GatewayInitializer::init(std::string const& _configPath)
{
    // init the log
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);

    m_logInitializer = std::make_shared<BoostLogInitializer>();
    m_logInitializer->initLog(pt);
    INIT_LOG(INFO) << LOG_DESC("initLog success");

    INIT_LOG(INFO) << LOG_DESC("initGateway: ") << _configPath;
    auto config = std::make_shared<PPCConfig>();

    config->loadGatewayConfig(_configPath, true);
    auto threadPool = std::make_shared<bcos::ThreadPool>(
        "gateway", config->gatewayConfig().networkConfig.threadPoolSize);

    GatewayFactory gatewayFactory(config);
    // default 1min
    m_healthChecker = std::make_shared<HealthCheckTimer>(60 * 1000);
    m_gateway = gatewayFactory.build(
        std::make_shared<RemoteFrontBuilder>(config->grpcConfig(), m_healthChecker));

    m_server = std::make_shared<GrpcServer>(config->gatewayConfig().grpcServerConfig);
    // register the gateway service
    auto gatewayService = std::make_shared<GatewayServer>(m_gateway,
        std::make_shared<MessageOptionalHeaderBuilderImpl>(), std::make_shared<NodeInfoFactory>());
    m_server->registerService(gatewayService);
}

void GatewayInitializer::start()
{
    if (m_gateway)
    {
        m_gateway->start();
    }
    if (m_server)
    {
        m_server->start();
    }
    if (m_healthChecker)
    {
        m_healthChecker->start();
    }
}
void GatewayInitializer::stop()
{
    if (m_healthChecker)
    {
        m_healthChecker->stop();
    }
    if (m_server)
    {
        m_server->stop();
    }
    if (m_gateway)
    {
        m_gateway->stop();
    }
}