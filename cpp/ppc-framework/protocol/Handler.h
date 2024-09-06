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
 * @file EndPoint.h
 * @author: yujiechen
 * @date 2024-08-22
 */

#pragma once
#include <memory>
#include <string>
#include <vector>

namespace ppc::protocol
{
struct HealthCheckHandler
{
    using Ptr = std::shared_ptr<HealthCheckHandler>;
    HealthCheckHandler(std::string const& _serviceName) : serviceName(_serviceName) {}

    std::string serviceName;
    // handler used to check the health
    std::function<bool()> checkHealthHandler;
    // handler called when the service un-health
    std::function<void()> onUnHealthHandler;
    // remove the handler when the service un-health
    bool removeHandlerOnUnhealth = true;
};
}  // namespace ppc::protocol