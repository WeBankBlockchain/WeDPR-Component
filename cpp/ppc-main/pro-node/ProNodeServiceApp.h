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
 * @file ProNodeServiceApp.h
 * @author: yujiechen
 * @date 2022-11-25
 */
#pragma once
#include "libinitializer/Initializer.h"
#include <bcos-utilities/BoostLogInitializer.h>
#include <servant/Application.h>

#define NodeService_LOG(LEVEL) BCOS_LOG(LEVEL) << "[NodeServiceApp]"

namespace ppc::rpc
{
class Rpc;
}
namespace ppctars
{
class ProNodeServiceApp : public tars::Application
{
public:
    ProNodeServiceApp() {}
    ~ProNodeServiceApp() override{};

    void destroyApp() override;
    void initialize() override;

protected:
    virtual void initService(std::string const& _configPath);

private:
    std::string m_configPath;
    bcos::BoostLogInitializer::Ptr m_logInitializer;
    ppc::initializer::Initializer::Ptr m_nodeInitializer;
    // TODO: rpc support pro-mode
    std::shared_ptr<ppc::rpc::Rpc> m_rpc;
};
}  // namespace ppctars