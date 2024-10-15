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
 * @file CEMInitializer.cpp
 * @author: caryliao
 * @date 2022-11-19
 */
#include "CEMInitializer.h"
#include "ppc-cem/src/CEMService.h"

using namespace ppc::rpc;
using namespace bcos;
using namespace ppc::cem;
using namespace ppc::tools;

void CEMInitializer::init(std::string const& _configPath)
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
    ppcConfig->loadRpcConfig(pt);
    ppcConfig->loadCEMConfig(pt);
    // bool useMysql = pt.get<bool>("cem.use_mysql", false);
    auto storageConfig = ppcConfig->storageConfig();
    auto cemConfig = ppcConfig->cemConfig();
    auto rpcFactory = std::make_shared<RpcFactory>(ppcConfig->agencyID());
    m_rpc = rpcFactory->buildRpc(ppcConfig, nullptr);
    auto cemService = std::make_shared<CEMService>();
    cemService->setCEMConfig(cemConfig);
    cemService->setStorageConfig(storageConfig);
    m_rpc->registerHandler("match", std::bind(&CEMService::makeCiphertextEqualityMatchRpc,
                                        cemService, std::placeholders::_1, std::placeholders::_2));
    m_rpc->registerHandler("encrypt", std::bind(&CEMService::encryptDatasetRpc, cemService,
                                          std::placeholders::_1, std::placeholders::_2));

    INIT_LOG(INFO) << LOG_DESC("init the rpc success");
}

void CEMInitializer::start()
{
    // start the ppc cem
    if (m_rpc)
    {
        m_rpc->start();
    }
}

void CEMInitializer::stop()
{
    if (m_rpc)
    {
        m_rpc->stop();
    }
}
