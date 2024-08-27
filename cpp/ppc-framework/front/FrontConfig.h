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
#include <memory>
#include <string>
#include <vector>

namespace ppc::front
{
/**
 * @brief the gateway endpoint
 *
 */
class GatewayEndPoint
{
public:
    GatewayEndPoint() = default;
    GatewayEndPoint(std::string const& host, uint16_t port) : m_host(std::move(host)), m_port(port)
    {}
    virtual ~GatewayEndPoint() = default;

    virtual std::string const& host() const { return m_host; }
    uint16_t port() const { return m_port; }

    void setHost(std::string host) { m_host = std::move(host); }
    void setPort(uint16_t port) { m_port = port; }

private:
    // the host
    std::string m_host;
    // the port
    uint16_t m_port;
};

// Note: swig explosed interface
class FrontConfig
{
public:
    using Ptr = std::shared_ptr<FrontConfig>;
    FrontConfig(int threadPoolSize, std::string agencyID)
      : m_threadPoolSize(threadPoolSize), m_agencyID(std::move(agencyID))
    {}
    virtual ~FrontConfig() = default;

    virtual int threadPoolSize() const { return m_threadPoolSize; }
    virtual std::string const agencyID() const { return m_agencyID; }
    virtual std::vector<GatewayEndPoint> const& gatewayInfo() const { return m_gatewayInfo; }
    virtual void setGatewayInfo(std::vector<GatewayEndPoint> gatewayInfo)
    {
        m_gatewayInfo = std::move(gatewayInfo);
    }

    virtual void appendGatewayInfo(GatewayEndPoint&& endpoint)
    {
        // TODO:check the endpoint
        m_gatewayInfo.push_back(endpoint);
    }

private:
    int m_threadPoolSize;
    std::string m_agencyID;
    std::vector<GatewayEndPoint> m_gatewayInfo;
};

class FrontConfigBuilder
{
public:
    using Ptr = std::shared_ptr<FrontConfigBuilder>;
    FrontConfigBuilder() = default;
    virtual ~FrontConfigBuilder() = default;

    FrontConfig::Ptr build(int threadPoolSize, std::string agencyID)
    {
        return std::make_shared<FrontConfig>(threadPoolSize, agencyID);
    }
};
}  // namespace ppc::front