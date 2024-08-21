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
 * @author: shawnhe
 * @date 2022-10-21
 */

#pragma once

#include <ppc-framework/gateway/GatewayInterface.h>
#include <ppc-gateway/ppc-gateway/Gateway.h>

namespace ppctars
{
class GatewayInitializer
{
public:
    using Ptr = std::shared_ptr<GatewayInitializer>;

    explicit GatewayInitializer(ppc::protocol::NodeArch _arch, ppc::tools::PPCConfig::Ptr _config,
        ppc::storage::CacheStorage::Ptr _cache, ppc::front::PPCMessageFactory::Ptr _messageFactory,
        std::shared_ptr<bcos::ThreadPool> _threadPool)
      : m_gatewayConfig(std::move(_config)),
        m_cache(std::move(_cache)),
        m_messageFactory(std::move(_messageFactory)),
        m_threadPool(std::move(_threadPool))

    {
        init(_arch);
    }

    virtual ~GatewayInitializer() { stop(); }

    void start();
    void stop();

public:
    ppc::storage::CacheStorage::Ptr cache() { return m_cache; }

    ppc::front::PPCMessageFactory::Ptr messageFactory() { return m_messageFactory; }
    std::shared_ptr<bcos::ThreadPool> threadPool() { return m_threadPool; }

    ppc::gateway::GatewayInterface::Ptr gateway() { return m_gateway; }
    ppc::gateway::GatewayFactory::Ptr gatewayFactory() { return m_gatewayFactory; }

protected:
    void init(ppc::protocol::NodeArch _arch);

private:
    ppc::tools::PPCConfig::Ptr m_gatewayConfig;
    ppc::storage::CacheStorage::Ptr m_cache;

    ppc::front::PPCMessageFactory::Ptr m_messageFactory;

    std::shared_ptr<bcos::ThreadPool> m_threadPool;

    ppc::gateway::GatewayInterface::Ptr m_gateway;
    ppc::gateway::GatewayFactory::Ptr m_gatewayFactory;

    std::atomic_bool m_running = {false};
};
}  // namespace ppctars
