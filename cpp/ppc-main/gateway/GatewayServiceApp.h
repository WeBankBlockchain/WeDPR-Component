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
 * @file GatewayServiceApp.h
 * @author: shawnhe
 * @date 2022-10-21
 */
#pragma once
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-tars-protocol/Common.h"
#include "ppc-tars-service/GatewayService/GatewayInitializer.h"
#include <bcos-utilities/BoostLogInitializer.h>
#include <servant/Application.h>

#define GATEWAYAPP_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GatewayServiceApp]"

namespace ppctars
{
class GatewayServiceApp : public tars::Application
{
public:
    GatewayServiceApp() {}
    ~GatewayServiceApp() override{};

    void destroyApp() override
    {
        if (m_gatewayInitializer)
        {
            m_gatewayInitializer->stop();
        }
    }
    void initialize() override;

protected:
    virtual void initService(std::string const& _configPath);

private:
    std::string m_configPath;
    bcos::BoostLogInitializer::Ptr m_logInitializer;
    ppctars::GatewayInitializer::Ptr m_gatewayInitializer;
};
}  // namespace ppctars