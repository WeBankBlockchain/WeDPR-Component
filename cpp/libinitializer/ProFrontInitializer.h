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
 * @file ProFrontInitializer.h
 * @author: yujiechen
 * @date 2022-11-25
 */
#pragma once
#include "FrontInitializer.h"
#include "ppc-framework/gateway/GatewayInterface.h"
#include <bcos-utilities/Timer.h>

namespace ppc::initializer
{
class ProFrontInitializer : public FrontInitializer
{
public:
    using Ptr = std::shared_ptr<ProFrontInitializer>;
    ProFrontInitializer(const std::string& _selfAgencyId,
        std::shared_ptr<bcos::ThreadPool> _threadPool,
        front::PPCMessageFaceFactory::Ptr _messageFactory)
      : FrontInitializer(_selfAgencyId, _threadPool, _messageFactory),
        m_statusReporter(std::make_shared<bcos::Timer>(3000, "frontReporter"))
    {}
    ~ProFrontInitializer() override { stop(); }

    void start() override
    {
        FrontInitializer::start();
        if (m_statusReporter)
        {
            m_statusReporter->registerTimeoutHandler(
                boost::bind(&ProFrontInitializer::reportStatusToGateway, this));
            m_statusReporter->start();
        }
    }

    void stop() override
    {
        if (m_statusReporter)
        {
            m_statusReporter->stop();
        }
        FrontInitializer::stop();
    }

protected:
    // report the endPoint to the gateway periodically
    // Note: must set gatewayInterface into the front before call this function
    virtual void reportStatusToGateway()
    {
        try
        {
            m_front->gatewayInterface()->registerFront(m_front->selfEndPoint(), nullptr);
        }
        catch (std::exception const& e)
        {
            FRONT_LOG(WARNING) << LOG_DESC("reportStatusToGateway exception")
                               << LOG_KV("exception", boost::diagnostic_information(e));
        }
        m_statusReporter->restart();
    }

private:
    std::shared_ptr<bcos::Timer> m_statusReporter;
};
}  // namespace ppc::initializer
