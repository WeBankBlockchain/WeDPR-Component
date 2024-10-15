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
 * @file GrpcConfig.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "ppc-framework/protocol/EndPoint.h"
#include <memory>
#include <string>

namespace ppc::protocol
{
class GrpcServerConfig
{
public:
    using Ptr = std::shared_ptr<GrpcServerConfig>;
    GrpcServerConfig() = default;
    GrpcServerConfig(EndPoint endPoint, bool enableHealthCheck)
      : m_endPoint(std::move(endPoint)), m_enableHealthCheck(enableHealthCheck)
    {}
    std::string listenEndPoint() const { return m_endPoint.listenEndPoint(); }

    void setEndPoint(EndPoint endPoint) { m_endPoint = endPoint; }
    void setEnableHealthCheck(bool enableHealthCheck) { m_enableHealthCheck = enableHealthCheck; }

    EndPoint const& endPoint() const { return m_endPoint; }
    EndPoint& mutableEndPoint() { return m_endPoint; }
    bool enableHealthCheck() const { return m_enableHealthCheck; }

protected:
    ppc::protocol::EndPoint m_endPoint;
    bool m_enableHealthCheck = true;
};
class GrpcConfig
{
public:
    using Ptr = std::shared_ptr<GrpcConfig>;
    GrpcConfig() = default;
    virtual ~GrpcConfig() = default;

    std::string const& loadBalancePolicy() const { return m_loadBalancePolicy; }
    void setLoadBalancePolicy(std::string const& loadBalancePolicy)
    {
        m_loadBalancePolicy = loadBalancePolicy;
    }

    bool enableHealthCheck() const { return m_enableHealthCheck; }
    void setEnableHealthCheck(bool enableHealthCheck) { m_enableHealthCheck = enableHealthCheck; }
    void setEnableDnslookup(bool enableDnslookup) { m_enableDnslookup = enableDnslookup; }

    bool enableDnslookup() const { return m_enableDnslookup; }

protected:
    bool m_enableHealthCheck = true;
    std::string m_loadBalancePolicy = "round_robin";
    bool m_enableDnslookup = false;
};
}  // namespace ppc::protocol