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
 * @file HealthCheckTimer.cpp
 * @author: yujiechen
 * @date 2024-09-6
 */
#include "HealthCheckTimer.h"
#include "Common.h"

using namespace ppc::protocol;
using namespace ppc::front;

HealthCheckTimer::HealthCheckTimer(int periodMs) : m_periodMs(periodMs)
{
    m_timer = std::make_shared<bcos::Timer>(m_periodMs, "healthChecker");
}

void HealthCheckTimer::start()
{
    if (m_running == true)
    {
        HEALTH_LOG(INFO) << LOG_DESC("HealthCheckTimer has already been started!");
        return;
    }
    m_running = true;

    auto self = weak_from_this();
    m_timer->registerTimeoutHandler([self]() {
        auto healthChecker = self.lock();
        if (!healthChecker)
        {
            return;
        }
        healthChecker->checkHealth();
    });
    if (m_timer)
    {
        m_timer->start();
    }
    HEALTH_LOG(INFO) << LOG_DESC("start the HealthCheckTimer success");
}

void HealthCheckTimer::stop()
{
    if (m_running == false)
    {
        HEALTH_LOG(INFO) << LOG_DESC("HealthCheckTimer has already been stopped!");
        return;
    }
    m_running = false;
    if (m_timer)
    {
        m_timer->stop();
    }
    HEALTH_LOG(INFO) << LOG_DESC("stop the HealthCheckTimer success");
}


void HealthCheckTimer::registerHealthCheckHandler(HealthCheckHandler::Ptr healthCheckHandler)
{
    if (!healthCheckHandler)
    {
        return;
    }
    bcos::WriteGuard l(x_healthCheckHandlers);
    m_healthCheckHandlers[healthCheckHandler->serviceName] = healthCheckHandler;
    HEALTH_LOG(INFO) << LOG_DESC("registerHealthCheckHandler for ")
                     << healthCheckHandler->serviceName;
}


void HealthCheckTimer::checkHealth()
{
    try
    {
        std::map<std::string, HealthCheckHandler::Ptr> handlers;
        {
            bcos::ReadGuard l(x_healthCheckHandlers);
            handlers = m_healthCheckHandlers;
        }
        std::set<std::string> serviceToRemove;
        for (auto const& it : handlers)
        {
            auto handler = it.second;
            bool health = true;
            if (handler->checkHealthHandler)
            {
                health = handler->checkHealthHandler();
            }
            if (health)
            {
                continue;
            }
            HEALTH_LOG(WARNING) << LOG_DESC("Detect unHealth service: ") << it.first;
            if (handler->onUnHealthHandler)
            {
                handler->onUnHealthHandler();
            }
            if (handler->removeHandlerOnUnhealth)
            {
                serviceToRemove.insert(it.first);
            }
        }
        m_timer->restart();
        if (serviceToRemove.empty())
        {
            return;
        }
        {
            bcos::WriteGuard l(x_healthCheckHandlers);
            for (auto const& service : serviceToRemove)
            {
                m_healthCheckHandlers.erase(service);
            }
        }
    }
    catch (std::exception const& e)
    {
        HEALTH_LOG(WARNING) << LOG_DESC("checkHealth exception, error: ")
                            << LOG_KV("error", boost::diagnostic_information(e));
        m_timer->restart();
    }
}
