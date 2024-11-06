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
 * @file NodeDiscovery.h
 * @author: yujiechen
 * @date 2024-11-06
 */
#pragma once
#include "ppc-framework/front/INodeDiscovery.h"
#include "ppc-framework/gateway/IGateway.h"
#include <bcos-utilities/Timer.h>
#include <memory>

namespace ppc::front
{
class NodeDiscovery : public INodeDiscovery
{
public:
    using Ptr = std::shared_ptr<NodeDiscovery>;
    NodeDiscovery(ppc::gateway::IGateway::Ptr gatewayClient);
    ~NodeDiscovery() override = default;

    std::vector<std::shared_ptr<ppc::protocol::INodeInfo>> getAliveNodeList() const override;
    void start() override;
    void stop() override;

protected:
    virtual void fetchMetaInfoFromGateway();

private:
    std::shared_ptr<bcos::Timer> m_metaFetcher;
    ppc::gateway::IGateway::Ptr m_gatewayClient;

    std::vector<ppc::protocol::INodeInfo::Ptr> m_aliveNodeList;
    mutable bcos::SharedMutex x_aliveNodeList;
};
}  // namespace ppc::front