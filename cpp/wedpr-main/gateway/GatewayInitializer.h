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
 * @file GatewayInitializer.h
 * @author: yujiechen
 * @date 2022-11-14
 */
#pragma once
#include "ppc-framework/gateway/IGateway.h"
#include <bcos-utilities/BoostLogInitializer.h>
#include <bcos-utilities/Log.h>
#include <memory>

#define INIT_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("GATEWAYInit")
namespace ppc::protocol
{
class GrpcServer;
}
namespace ppc::protocol
{
class HealthCheckTimer;
};
namespace ppc::gateway
{
class GatewayInitializer
{
public:
    using Ptr = std::shared_ptr<GatewayInitializer>;
    GatewayInitializer() = default;
    virtual ~GatewayInitializer() { stop(); }

    virtual void init(std::string const& _configPath);
    virtual void start();
    virtual void stop();

protected:
    bcos::BoostLogInitializer::Ptr m_logInitializer;
    ppc::gateway::IGateway::Ptr m_gateway;
    std::shared_ptr<ppc::protocol::HealthCheckTimer> m_healthChecker;
    std::shared_ptr<ppc::protocol::GrpcServer> m_server;
};
}  // namespace ppc::gateway