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
struct GrpcServerConfig
{
    ppc::protocol::EndPoint endPoint;

    std::string listenEndPoint() const { return endPoint.listenEndPoint(); }
};
class GrpcConfig
{
public:
    using Ptr = std::shared_ptr<GrpcConfig>;
    GrpcConfig() = default;
    virtual ~GrpcConfig() = default;

    std::string const& loadBalancePolicy() const { return m_loadBanlancePolicy; }
    void setLoadBalancePolicy(std::string const& loadBanlancePolicy)
    {
        m_loadBanlancePolicy = loadBanlancePolicy;
    }

private:
    std::string m_loadBanlancePolicy = "round_robin";
};
}  // namespace ppc::protocol