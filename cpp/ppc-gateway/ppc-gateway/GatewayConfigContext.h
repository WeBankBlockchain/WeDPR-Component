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
 * @file GatewayConfigContext.h
 * @author: shawnhe
 * @date 2022-10-23
 */

#pragma once

#include "Common.h"
#include "ppc-framework/storage/CacheStorage.h"
#include "ppc-storage/src/redis/RedisStorage.h"
#include <bcos-boostssl/context/ContextConfig.h>
#include <ppc-tools/src/config/PPCConfig.h>
#include <boost/property_tree/ptree.hpp>
#include <unordered_map>
#include <utility>

namespace ppc::gateway
{
class GatewayConfigContext
{
public:
    using Ptr = std::shared_ptr<GatewayConfigContext>;
    GatewayConfigContext(ppc::tools::PPCConfig::Ptr _config) : m_config(_config)
    {
        if (!m_config->gatewayConfig().networkConfig.disableSsl)
        {
            GATEWAY_LOG(INFO) << LOG_DESC("GatewayConfigContext: initContextConfig");
            initContextConfig();
            GATEWAY_LOG(INFO) << LOG_DESC("GatewayConfigContext: initContextConfig success");
        }
    }
    virtual ~GatewayConfigContext() = default;
    [[nodiscard]] std::shared_ptr<bcos::boostssl::context::ContextConfig> contextConfig() const
    {
        return m_contextConfig;
    }
    ppc::tools::PPCConfig::Ptr const& config() const { return m_config; }

private:
    void initContextConfig();

private:
    ppc::tools::PPCConfig::Ptr m_config;
    std::shared_ptr<bcos::boostssl::context::ContextConfig> m_contextConfig;
};

}  // namespace ppc::gateway