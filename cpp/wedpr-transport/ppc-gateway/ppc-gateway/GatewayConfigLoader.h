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
 * @file GatewayConfigLoader.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "bcos-boostssl/interfaces/NodeInfoDef.h"

namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::gateway
{
class GatewayConfigLoader
{
public:
    using EndPointSet = std::set<bcos::boostssl::NodeIPEndpoint>;
    using Ptr = std::shared_ptr<GatewayConfigLoader>;
    GatewayConfigLoader(std::shared_ptr<ppc::tools::PPCConfig> config)
      : m_config(std::move(config)), m_nodeIPEndpointSet(std::make_shared<EndPointSet>())
    {
        loadP2pConnectedNodes();
    }
    virtual ~GatewayConfigLoader() = default;

    EndPointSet const& nodeIPEndpointSet() const { return *m_nodeIPEndpointSet; }

    std::shared_ptr<EndPointSet> const& nodeIPEndpointSetPtr() const { return m_nodeIPEndpointSet; }

protected:
    void parseConnectedJson(const std::string& _json, EndPointSet& nodeIPEndpointSet);
    void loadP2pConnectedNodes();
    void hostAndPort2Endpoint(const std::string& _host, bcos::boostssl::NodeIPEndpoint& _endpoint);
    bool isValidPort(int port);

private:
    std::shared_ptr<ppc::tools::PPCConfig> m_config;
    std::shared_ptr<EndPointSet> m_nodeIPEndpointSet;
};
}  // namespace ppc::gateway