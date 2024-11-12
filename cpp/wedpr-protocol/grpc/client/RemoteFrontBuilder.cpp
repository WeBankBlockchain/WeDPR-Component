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
 * @file RemoteFrontBuilder.cpp
 * @author: yujiechen
 * @date 2024-09-4
 */
#include "RemoteFrontBuilder.h"
// Note: it's better not to include generated grpc files in the header, since it will slow the
// compiler speed
#include "FrontClient.h"

using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::protocol;

IFrontClient::Ptr RemoteFrontBuilder::buildClient(std::string endPoint,
    std::function<void()> onUnHealthHandler, bool removeHandlerOnUnhealth) const
{
    auto frontClient = std::make_shared<FrontClient>(m_grpcConfig, endPoint);
    if (m_healthChecker)
    {
        auto healthCheckHandler = std::make_shared<HealthCheckHandler>("front_" + endPoint);
        healthCheckHandler->checkHealthHandler = [frontClient]() {
            return frontClient->checkHealth();
        };
        healthCheckHandler->onUnHealthHandler = onUnHealthHandler;
        healthCheckHandler->removeHandlerOnUnhealth = removeHandlerOnUnhealth;
        m_healthChecker->registerHealthCheckHandler(healthCheckHandler);
    }
    return frontClient;
}
