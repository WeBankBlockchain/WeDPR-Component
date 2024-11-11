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
#include "Common.h"
#include "wedpr-protocol/grpc/client/GatewayClient.h"
#include "wedpr-protocol/grpc/server/FrontServer.h"
#include "wedpr-protocol/grpc/server/GrpcServer.h"

using namespace ppc::front;
using namespace ppc::protocol;
using namespace ppc::sdk;


ProTransportImpl::ProTransportImpl(ppc::front::FrontConfig::Ptr config, int keepAlivePeriodMs)
  : Transport(config), m_keepAlivePeriodMs(keepAlivePeriodMs)
{
    // Note: since the config has been moved away, should not use the `config`, use `m_config`
    // instead default enable health-check
    auto grpcServerConfig = std::make_shared<GrpcServerConfig>(m_config->selfEndPoint(), true);
    m_server = std::make_shared<GrpcServer>(grpcServerConfig);

    FrontFactory frontFactory;
    auto nodeInfoFactory = std::make_shared<NodeInfoFactory>();
    m_gateway = std::make_shared<GatewayClient>(
        m_config->grpcConfig(), m_config->gatewayGrpcTarget(), nodeInfoFactory);
    m_front = frontFactory.build(
        nodeInfoFactory, m_msgPayloadBuilder, m_routeInfoBuilder, m_gateway, m_config);

    m_frontService = std::make_shared<FrontServer>(m_msgBuilder, m_front);
    // register the frontService
    m_server->registerService(m_frontService);
}

ProTransportImpl::~ProTransportImpl()
{
    TRANSPORT_LOG(INFO) << LOG_DESC("stop pro transport");
    stop();
    TRANSPORT_LOG(INFO) << LOG_DESC("stop pro transport success");
}

void ProTransportImpl::start()
{
    m_timer = std::make_shared<bcos::Timer>(m_keepAlivePeriodMs, "frontKeepAlive");
    auto self = weak_from_this();
    m_timer->registerTimeoutHandler([self]() {
        auto transport = self.lock();
        if (!transport)
        {
            return;
        }
        transport->keepAlive();
    });
    if (m_timer)
    {
        m_timer->start();
    }
    if (m_server)
    {
        m_server->start();
    }
    if (m_frontService)
    {
        // Note: the server is inited after start
        m_frontService->setHealthCheckService(m_server->server()->GetHealthCheckService());
    }
    if (m_front)
    {
        m_front->start();
    }
}

void ProTransportImpl::stop()
{
    if (m_timer)
    {
        m_timer->stop();
    }
    if (m_server)
    {
        m_server->stop();
    }
    if (m_front)
    {
        m_front->stop();
    }
}

void ProTransportImpl::keepAlive()
{
    try
    {
        m_gateway->registerNodeInfo(m_front->nodeInfo());
    }
    catch (std::exception const& e)
    {
        TRANSPORT_LOG(WARNING) << LOG_DESC("keepAlive exception")
                               << LOG_KV("error", boost::diagnostic_information(e));
    }
    m_timer->restart();
}