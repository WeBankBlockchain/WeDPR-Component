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
 * @file TransportBuilder.h
 * @author: yujiechen
 * @date 2024-09-04
 */
#pragma once
#include "Transport.h"
#include "ppc-framework/front/FrontConfig.h"
#include "ppc-framework/gateway/IGateway.h"
#include <bcos-utilities/BoostLogInitializer.h>
#include <memory>
namespace ppc::sdk
{
enum class SDKMode : uint8_t
{
    AIR = 0x00,
    PRO = 0x01,
};
class TransportBuilder
{
public:
    using Ptr = std::shared_ptr<TransportBuilder>;
    TransportBuilder();
    virtual ~TransportBuilder() = default;

    Transport::Ptr build(
        SDKMode mode, ppc::front::FrontConfig::Ptr config, ppc::gateway::IGateway::Ptr gateway);

    // Note: the swig-wrapper can't receive the null shared_ptr
    Transport::Ptr buildProTransport(ppc::front::FrontConfig::Ptr config)
    {
        return build(SDKMode::PRO, config, nullptr);
    }

    ppc::front::FrontConfig::Ptr buildConfig(int threadPoolSize, std::string nodeID);

    ppc::front::FrontConfigBuilder::Ptr const& frontConfigBuilder() { return m_frontConfigBuilder; }


    void initLog(const std::string& configPath);

private:
    ppc::front::FrontConfigBuilder::Ptr m_frontConfigBuilder;
    bcos::BoostLogInitializer::Ptr m_logInitializer;
    bool m_logInited = false;
    bcos::Mutex x_logInited;
};
}  // namespace ppc::sdk