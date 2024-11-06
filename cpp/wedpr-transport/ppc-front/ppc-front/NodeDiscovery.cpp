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
 * @file NodeDiscovery.cpp
 * @author: yujiechen
 * @date 2024-11-06
 */
#include "NodeDiscovery.h"
#include "Common.h"

using namespace ppc::front;

NodeDiscovery::NodeDiscovery(ppc::gateway::IGateway::Ptr gatewayClient)
  : m_gatewayClient(std::move(gatewayClient))
{
    // fetch the meta information every 10s
    m_metaFetcher = std::make_shared<bcos::Timer>(10 * 1000, "metaFetcher");
    m_metaFetcher->registerTimeoutHandler([this]() { fetchMetaInfoFromGateway(); });
}

std::vector<ppc::protocol::INodeInfo::Ptr> NodeDiscovery::getAliveNodeList() const
{
    bcos::ReadGuard l(x_aliveNodeList);
    return m_aliveNodeList;
}

void NodeDiscovery::start()
{
    if (m_metaFetcher)
    {
        m_metaFetcher->start();
    }
}

void NodeDiscovery::stop()
{
    if (m_metaFetcher)
    {
        m_metaFetcher->stop();
    }
}

void NodeDiscovery::fetchMetaInfoFromGateway()
{
    try
    {
        auto nodeList = m_gatewayClient->getAliveNodeList();
        bcos::WriteGuard l(x_aliveNodeList);
        m_aliveNodeList = nodeList;
    }
    catch (std::exception const& e)
    {
        FRONT_LOG(WARNING) << LOG_DESC("fetchMetaInfoFromGateway information failed")
                           << LOG_KV("error", boost::diagnostic_information(e));
    }
    m_metaFetcher->restart();
}