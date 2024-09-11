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
 * @file RemoteFrontBuilder.h
 * @author: yujiechen
 * @date 2024-09-4
 */
#pragma once
#include "HealthCheckTimer.h"
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/protocol/GrpcConfig.h"

namespace ppc::front
{
class RemoteFrontBuilder : public IFrontBuilder
{
public:
    using Ptr = std::shared_ptr<RemoteFrontBuilder>;
    RemoteFrontBuilder(ppc::protocol::GrpcConfig::Ptr const& grpcConfig,
        ppc::protocol::HealthCheckTimer::Ptr healthChecker)
      : m_grpcConfig(grpcConfig), m_healthChecker(healthChecker)
    {}
    ~RemoteFrontBuilder() override = default;

    IFrontClient::Ptr buildClient(std::string endPoint, std::function<void()> onUnHealthHandler,
        bool removeHandlerOnUnhealth) const override;

private:
    ppc::protocol::GrpcConfig::Ptr m_grpcConfig;
    ppc::protocol::HealthCheckTimer::Ptr m_healthChecker;
};
}  // namespace ppc::front