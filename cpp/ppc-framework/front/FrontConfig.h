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
 * @file FrontConfig.h
 * @author: yujiechen
 * @date 2024-08-22
 */

#pragma once
#include "ppc-framework/protocol/EndPoint.h"
#include <memory>
#include <string>
#include <vector>

namespace ppc::front
{
// Note: swig explosed interface
class FrontConfig
{
public:
    using Ptr = std::shared_ptr<FrontConfig>;
    FrontConfig(int threadPoolSize, std::string nodeID)
      : m_threadPoolSize(threadPoolSize), m_nodeID(std::move(nodeID))
    {}
    virtual ~FrontConfig() = default;

    virtual int threadPoolSize() const { return m_threadPoolSize; }
    virtual std::string const& nodeID() const { return m_nodeID; }
    virtual std::vector<ppc::protocol::EndPoint> const& gatewayInfo() const
    {
        return m_gatewayInfo;
    }
    virtual void setGatewayInfo(std::vector<ppc::protocol::EndPoint> gatewayInfo)
    {
        m_gatewayInfo = std::move(gatewayInfo);
    }

    virtual void appendGatewayInfo(ppc::protocol::EndPoint&& endpoint)
    {
        // TODO:check the endpoint
        m_gatewayInfo.push_back(endpoint);
    }

    ppc::protocol::EndPoint const& selfEndPoint() const { return m_selfEndPoint; }
    void setSelfEndPoint(ppc::protocol::EndPoint const& endPoint) { m_selfEndPoint = endPoint; }

    // TODO here
    std::string gatewayEndPoints() { return ""; }

    std::string const& loadBalancePolicy() const { return m_loadBanlancePolicy; }
    void setLoadBalancePolicy(std::string const& loadBanlancePolicy)
    {
        m_loadBanlancePolicy = loadBanlancePolicy;
    }

private:
    std::string m_loadBanlancePolicy = "round_robin";
    ppc::protocol::EndPoint m_selfEndPoint;
    int m_threadPoolSize;
    std::string m_nodeID;
    std::vector<ppc::protocol::EndPoint> m_gatewayInfo;
};

class FrontConfigBuilder
{
public:
    using Ptr = std::shared_ptr<FrontConfigBuilder>;
    FrontConfigBuilder() = default;
    virtual ~FrontConfigBuilder() = default;

    FrontConfig::Ptr build(int threadPoolSize, std::string nodeID)
    {
        return std::make_shared<FrontConfig>(threadPoolSize, nodeID);
    }
};
}  // namespace ppc::front