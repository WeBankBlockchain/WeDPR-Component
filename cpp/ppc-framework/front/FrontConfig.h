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
#include "ppc-framework/protocol/GrpcConfig.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include <memory>
#include <sstream>
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
        m_gatewayInfo.push_back(endpoint);
    }

    ppc::protocol::EndPoint const& selfEndPoint() const { return m_selfEndPoint; }
    void setSelfEndPoint(ppc::protocol::EndPoint const& endPoint) { m_selfEndPoint = endPoint; }

    // refer to: https://github.com/grpc/grpc-node/issues/2066
    // grpc prefer to using ipv4:${host1}:${port1},${host2}:${port2} as target to support multiple
    // servers
    std::string gatewayGrpcTarget()
    {
        std::stringstream oss;
        oss << "ipv4:";
        for (auto const& endPoint : m_gatewayInfo)
        {
            oss << endPoint.entryPoint() << ",";
        }
        return oss.str();
    }

    void setGrpcConfig(ppc::protocol::GrpcConfig::Ptr grpcConfig)
    {
        m_grpcConfig = std::move(grpcConfig);
    }
    ppc::protocol::GrpcConfig::Ptr const& grpcConfig() const { return m_grpcConfig; }

    // generate the nodeInfo
    virtual ppc::protocol::INodeInfo::Ptr generateNodeInfo() const = 0;

    virtual std::vector<std::string> const& getComponents() const { return m_components; }
    void setComponents(std::vector<std::string> const& components) { m_components = components; }

protected:
    ppc::protocol::GrpcConfig::Ptr m_grpcConfig;
    ppc::protocol::EndPoint m_selfEndPoint;
    int m_threadPoolSize;
    std::string m_nodeID;
    std::vector<ppc::protocol::EndPoint> m_gatewayInfo;
    std::vector<std::string> m_components;
};

inline std::string printFrontDesc(FrontConfig::Ptr const& config)
{
    if (!config)
    {
        return "nullptr";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("endPoint", config->selfEndPoint().entryPoint())
                 << LOG_KV("nodeID", config->nodeID())
                 << LOG_KV("poolSize", config->threadPoolSize())
                 << LOG_KV("target", config->gatewayGrpcTarget());
    return stringstream.str();
}
}  // namespace ppc::front