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
    FrontConfig() = default;

    FrontConfig(int threadPoolSize, std::string nodeID)
      : m_threadPoolSize(threadPoolSize), m_nodeID(std::move(nodeID))
    {}
    virtual ~FrontConfig() = default;

    virtual int threadPoolSize() const { return m_threadPoolSize; }
    virtual void setThreadPoolSize(int threadPoolSize) { m_threadPoolSize = threadPoolSize; }

    virtual std::string const& nodeID() const { return m_nodeID; }
    virtual void setNodeID(std::string const& nodeID) { m_nodeID = nodeID; }

    ppc::protocol::EndPoint const& selfEndPoint() const { return m_selfEndPoint; }
    ppc::protocol::EndPoint& mutableSelfEndPoint() { return m_selfEndPoint; }

    void setSelfEndPoint(ppc::protocol::EndPoint const& endPoint) { m_selfEndPoint = endPoint; }

    void setGatewayGrpcTarget(std::string const& gatewayGrpcTarget)
    {
        m_gatewayGrpcTarget = gatewayGrpcTarget;
    }
    // refer to: https://github.com/grpc/grpc-node/issues/2066
    // grpc prefer to using ipv4:${host1}:${port1},${host2}:${port2} as target to support multiple
    // servers
    std::string const& gatewayGrpcTarget() const { return m_gatewayGrpcTarget; }

    void setGrpcConfig(ppc::protocol::GrpcConfig::Ptr grpcConfig)
    {
        m_grpcConfig = std::move(grpcConfig);
    }
    ppc::protocol::GrpcConfig::Ptr const& grpcConfig() const { return m_grpcConfig; }

    // generate the nodeInfo
    virtual ppc::protocol::INodeInfo::Ptr generateNodeInfo() const = 0;

    virtual std::vector<std::string> const& getComponents() const { return m_components; }
    void setComponents(std::vector<std::string> const& components) { m_components = components; }

    void addComponent(std::string const& component) { m_components.emplace_back(component); }

    std::vector<std::string>& mutableComponents() { return m_components; }

    std::string const meta() const { return m_meta; }
    void setMeta(std::string meta) { m_meta = std::move(meta); }

protected:
    ppc::protocol::GrpcConfig::Ptr m_grpcConfig;
    ppc::protocol::EndPoint m_selfEndPoint;
    int m_threadPoolSize;
    std::string m_nodeID;
    std::string m_gatewayGrpcTarget;
    std::vector<std::string> m_components;
    std::string m_meta;
};

class FrontConfigBuilder
{
public:
    using Ptr = std::shared_ptr<FrontConfigBuilder>;
    FrontConfigBuilder() = default;
    virtual ~FrontConfigBuilder() = default;

    virtual FrontConfig::Ptr build() const = 0;
    virtual FrontConfig::Ptr build(int threadPoolSize, std::string nodeID) const = 0;
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