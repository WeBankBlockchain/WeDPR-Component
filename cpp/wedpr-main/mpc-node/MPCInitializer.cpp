/*
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
 * @file MPCInitializer.cpp
 * @author: caryliao
 * @date 2023-03-24
 */
#include "MPCInitializer.h"
#include "ppc-mpc/src/MPCService.h"

using namespace ppc::rpc;
using namespace bcos;
using namespace ppc::mpc;
using namespace ppc::tools;

void MPCInitializer::init(std::string const& _configPath)
{
    // init the log
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);

    m_logInitializer = std::make_shared<BoostLogInitializer>();
    m_logInitializer->initLog(pt);
    INIT_LOG(INFO) << LOG_DESC("initLog success");

    // init the rpc
    INIT_LOG(INFO) << LOG_DESC("init the rpc");
    // load the rpc config
    auto ppcConfig = std::make_shared<PPCConfig>();
    // not specify the certPath in air-mode
    ppcConfig->loadRpcConfig(nullptr, pt);
    ppcConfig->loadMPCConfig(pt);
    // bool useMysql = pt.get<bool>("mpc.use_mysql", false);
    auto storageConfig = ppcConfig->storageConfig();
    auto mpcConfig = ppcConfig->mpcConfig();
    auto rpcFactory = std::make_shared<RpcFactory>(ppcConfig->agencyID());
    m_rpc = rpcFactory->buildRpc(ppcConfig);
    auto mpcService = std::make_shared<MPCService>();
    mpcService->setMPCConfig(mpcConfig);
    mpcService->setStorageConfig(storageConfig);
    m_rpc->registerHandler("run", std::bind(&MPCService::runMpcRpc, mpcService,
                                      std::placeholders::_1, std::placeholders::_2));
    m_rpc->registerHandler("kill", std::bind(&MPCService::killMpcRpc, mpcService,
                                       std::placeholders::_1, std::placeholders::_2));


    INIT_LOG(INFO) << LOG_DESC("init the mpc rpc success");
}

void MPCInitializer::start()
{
    // start the ppc mpc
    if (m_rpc)
    {
        m_rpc->start();
    }
}

void MPCInitializer::stop()
{
    if (m_rpc)
    {
        m_rpc->stop();
    }
}
