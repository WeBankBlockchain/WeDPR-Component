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
 * @file HealthCheckTimer.h
 * @author: yujiechen
 * @date 2024-09-6
 */
#pragma once
#include "ppc-framework/front/IFront.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Timer.h>
#include <memory>

namespace ppc::protocol
{
class HealthCheckTimer : public std::enable_shared_from_this<HealthCheckTimer>
{
public:
    using Ptr = std::shared_ptr<HealthCheckTimer>;
    HealthCheckTimer(int periodMs);
    virtual ~HealthCheckTimer() = default;

    virtual void start();
    virtual void stop();


    void registerHealthCheckHandler(HealthCheckHandler::Ptr healthCheckHandler);

protected:
    void checkHealth();

private:
    std::map<std::string, HealthCheckHandler::Ptr> m_healthCheckHandlers;
    mutable bcos::SharedMutex x_healthCheckHandlers;

    int m_periodMs = 3000;
    std::shared_ptr<bcos::Timer> m_timer;
    bool m_running = false;
};
}  // namespace ppc::protocol