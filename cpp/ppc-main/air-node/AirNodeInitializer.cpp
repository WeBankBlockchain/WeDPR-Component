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
 * @file AirNodeInitializer.cpp
 * @author: yujiechen
 * @date 2022-11-14
 */
#include "AirNodeInitializer.h"
#include "ppc-gateway/Gateway.h"
#include "ppc-gateway/GatewayConfigContext.h"
#include "ppc-rpc/src/RpcFactory.h"
#include "ppc-rpc/src/RpcMemory.h"
#include "ppc-storage/src/redis/RedisStorage.h"

using namespace ppc::protocol;
using namespace ppc::node;
using namespace ppc::gateway;
using namespace ppc::rpc;
using namespace ppc::storage;
using namespace ppc::initializer;
using namespace bcos;

void AirNodeInitializer::init(std::string const& _configPath)
{
    // init the log
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);

    m_logInitializer = std::make_shared<BoostLogInitializer>();
    m_logInitializer->initLog(pt);
    INIT_LOG(INFO) << LOG_DESC("initLog success");

    // init the node
    m_nodeInitializer = std::make_shared<Initializer>(_configPath);
    m_nodeInitializer->init(ppc::protocol::NodeArch::AIR);
    auto front = m_nodeInitializer->frontInitializer()->front();
    front->setSelfEndPoint("localhost");

    // init the gateway
    initGateway(_configPath);
    // set the gateway into front
    front->setGatewayInterface(m_gateway);

    INIT_LOG(INFO) << LOG_DESC("init the rpc");
    // load the rpc config
    // not specify the certPath in air-mode
    m_nodeInitializer->config()->loadRpcConfig(nullptr, pt);
    // init RpcStatusInterface
    RpcStatusInterface::Ptr rpcStatusInterface = std::make_shared<ppc::rpc::RpcMemory>(m_gateway);

    m_nodeInitializer->frontInitializer()->setRpcStatus(rpcStatusInterface);
    auto rpcFactory = std::make_shared<RpcFactory>(m_nodeInitializer->config()->agencyID());
    m_rpc = rpcFactory->buildRpc(m_nodeInitializer->config());
    m_rpc->setRpcStorage(rpcStatusInterface);
    m_rpc->setBsEcdhPSI(m_nodeInitializer->bsEcdhPsi());
    m_nodeInitializer->registerRpcHandler(m_rpc);

    // Note: only can fetchAgencyList after the gatewayInterface has been setted into front
    m_nodeInitializer->fetchAgencyList();
    INIT_LOG(INFO) << LOG_DESC("init the rpc success");
}

void AirNodeInitializer::initGateway(std::string const& _configPath)
{
    INIT_LOG(INFO) << LOG_DESC("initGateway: ") << _configPath;
    // not specify the certPath in air-mode
    auto config = m_nodeInitializer->config();
    config->loadGatewayConfig(ppc::protocol::NodeArch::AIR, nullptr, _configPath);

    auto threadPool = std::make_shared<bcos::ThreadPool>(
        "gateway", config->gatewayConfig().networkConfig.threadPoolSize);

    // Note: no need use redis as cache in-air-mode
    GatewayFactory gatewayFactory;
    auto gateway = gatewayFactory.buildGateway(ppc::protocol::NodeArch::AIR, config, nullptr,
        m_nodeInitializer->protocolInitializer()->ppcMsgFactory(), threadPool);
    auto frontInitializer = m_nodeInitializer->frontInitializer();
    gateway->registerFront(frontInitializer->front()->selfEndPoint(), frontInitializer->front());
    m_gateway = gateway;
}

void AirNodeInitializer::start()
{
    // start the node
    if (m_nodeInitializer)
    {
        m_nodeInitializer->start();
    }
    if (m_gateway)
    {
        m_gateway->start();
    }
    if (m_rpc)
    {
        m_rpc->start();
    }
}

void AirNodeInitializer::stop()
{
    if (m_rpc)
    {
        m_rpc->stop();
    }
    if (m_gateway)
    {
        m_gateway->stop();
    }
    if (m_nodeInitializer)
    {
        m_nodeInitializer->stop();
    }
}