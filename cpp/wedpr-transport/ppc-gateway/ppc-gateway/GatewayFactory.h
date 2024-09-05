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
 * @file GatewayFactory.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "GatewayConfigContext.h"
#include "GatewayConfigLoader.h"
#include "bcos-boostssl/websocket/WsConfig.h"
#include "gateway/GatewayImpl.h"
#include "ppc-gateway/p2p/Service.h"

namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::gateway
{
class GatewayFactory
{
public:
    using Ptr = std::shared_ptr<GatewayFactory>;
    GatewayFactory(std::shared_ptr<ppc::tools::PPCConfig> config) : m_config(std::move(config))
    {
        m_contextConfig = std::make_shared<GatewayConfigContext>(m_config);
        m_gatewayConfig = std::make_shared<GatewayConfigLoader>(m_config);
    }
    virtual ~GatewayFactory() = default;

    IGateway::Ptr build(ppc::front::IFrontBuilder::Ptr const& frontBuilder) const;

protected:
    Service::Ptr buildService() const;

private:
    std::shared_ptr<ppc::tools::PPCConfig> m_config;
    GatewayConfigContext::Ptr m_contextConfig;
    GatewayConfigLoader::Ptr m_gatewayConfig;
};
}  // namespace ppc::gateway