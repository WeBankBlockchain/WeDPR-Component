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
#include "ppc-framework/front/FrontConfig.h"
#include "ppc-framework/protocol/Constant.h"
#include "ppc-mpc/src/MPCService.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "wedpr-helper/ppc-utilities/Utilities.h"
#include "wedpr-protocol/protocol/src/ServiceConfig.h"
#include "wedpr-transport/sdk/src/TransportBuilder.h"

using namespace ppc::rpc;
using namespace bcos;
using namespace ppc::sdk;
using namespace ppc::mpc;
using namespace ppc::tools;
using namespace ppc::front;
using namespace ppc::protocol;

MPCInitializer::MPCInitializer() : m_transportBuilder(std::make_shared<TransportBuilder>()) {}

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
    m_config = std::make_shared<PPCConfig>();
    m_config->loadRpcConfig(pt);
    m_config->loadMPCConfig(pt);
    // bool useMysql = pt.get<bool>("mpc.use_mysql", false);
    auto storageConfig = m_config->storageConfig();
    auto mpcConfig = m_config->mpcConfig();
    auto rpcFactory = std::make_shared<RpcFactory>(m_config->agencyID());
    m_rpc = rpcFactory->buildRpc(m_config, nullptr);

    int threadPoolSize = mpcConfig.threadPoolSize;
    auto threadPool = std::make_shared<bcos::ThreadPool>("mpc-pool", threadPoolSize);

    INIT_LOG(INFO) << LOG_DESC("init the mpc threadpool")
                   << LOG_KV("threadPoolSize", threadPoolSize);

    auto mpcService = std::make_shared<MPCService>();
    mpcService->setMPCConfig(mpcConfig);
    mpcService->setStorageConfig(storageConfig);
    mpcService->setThreadPool(threadPool);

    m_rpc->registerHandler("run", std::bind(&MPCService::runMpcRpc, mpcService,
                                      std::placeholders::_1, std::placeholders::_2));
    m_rpc->registerHandler("asyncRun", std::bind(&MPCService::asyncRunMpcRpc, mpcService,
                                           std::placeholders::_1, std::placeholders::_2));
    m_rpc->registerHandler("kill", std::bind(&MPCService::killMpcRpc, mpcService,
                                       std::placeholders::_1, std::placeholders::_2));
    m_rpc->registerHandler("query", std::bind(&MPCService::queryMpcRpc, mpcService,
                                        std::placeholders::_1, std::placeholders::_2));
    INIT_LOG(INFO) << LOG_DESC("init the mpc rpc success");
    // init the transport
    initTransport(pt);
}

void MPCInitializer::initTransport(boost::property_tree::ptree const& property)
{
    INIT_LOG(INFO) << LOG_DESC("initTransport: load front config");
    m_config->loadFrontConfig(true, m_transportBuilder->frontConfigBuilder(), property);
    INIT_LOG(INFO) << LOG_DESC("initTransport: load front config success");

    // add the service meta
    ServiceConfigBuilder serviceConfigBuilder;
    auto serviceConfig = serviceConfigBuilder.buildServiceConfig();
    
    auto mpcEntryPoint =
    serviceConfigBuilder.buildEntryPoint(getServiceName(m_config->agencyID(), MPC_SERVICE_TYPE), m_config->accessEntrypoint());
    serviceConfig.addEntryPoint(mpcEntryPoint);

    auto spdzEntryPoint =
    serviceConfigBuilder.buildEntryPoint(getServiceName(m_config->agencyID(), SPDZ_SERVICE_TYPE), m_config->spdzConnectedEndPoint());
    serviceConfig.addEntryPoint(spdzEntryPoint);

    auto serviceMeta = serviceConfig.encode();
    m_config->frontConfig()->setMeta(serviceMeta);
    INIT_LOG(INFO) << LOG_DESC("initTransport: register serviceMeta")
                   << LOG_KV("serviceMeta", serviceMeta);
    INIT_LOG(INFO) << LOG_DESC("initTransport: buildProTransport");
    m_transport = m_transportBuilder->buildProTransport(m_config->frontConfig());
    INIT_LOG(INFO) << LOG_DESC("initTransport: buildProTransport success");
}

void MPCInitializer::start()
{
    // start the transport
    if (m_transport)
    {
        m_transport->start();
    }
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
    if (m_transport)
    {
        m_transport->stop();
    }
}
