/**
 *  Copyright (C) 2022 WeDPR.
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
 * @file FrontInitializer.cpp
 * @author: shawnhe
 * @date 2022-10-20
 */

#include "FrontInitializer.h"

using namespace ppctars;
using namespace ppc;
using namespace ppc::front;
using namespace ppc::protocol;
using namespace ppc::initializer;

void FrontInitializer::start()
{
    if (!m_running.exchange(true))
    {
        FRONT_LOG(INFO) << LOG_DESC("start the front");
        if (m_front)
        {
            m_front->start();
        }
        if (m_gatewayReporter)
        {
            m_gatewayReporter->registerTimeoutHandler(
                boost::bind(&FrontInitializer::reportOtherGateway, this));
            m_gatewayReporter->start();
        }
        FRONT_LOG(INFO) << LOG_DESC("start the front success");
    }
}

void FrontInitializer::stop()
{
    if (m_running.exchange(false))
    {
        FRONT_LOG(INFO) << LOG_DESC("stop the front");
        if (m_front)
        {
            m_front->stop();
        }
        if (m_gatewayReporter)
        {
            m_gatewayReporter->stop();
        }
        FRONT_LOG(INFO) << LOG_DESC("stop the front success");
    }
}

void FrontInitializer::init()
{
    FRONT_LOG(INFO) << LOG_BADGE("init front");

    auto ioService = std::make_shared<boost::asio::io_service>();
    // init front
    m_frontFactory = std::make_shared<FrontFactory>(m_selfAgencyId, m_threadPool);
    m_front = m_frontFactory->buildFront(ioService);
    FRONT_LOG(INFO) << LOG_BADGE("init front success");
}
