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
 * @file ProNodeServiceApp.cpp
 * @author: yujiechen
 * @date 2022-11-25
 */
#include "ProNodeServiceApp.h"
#include "GatewayService.h"
#include "ppc-rpc/src/RpcFactory.h"
#include "ppc-rpc/src/RpcMemory.h"
#include "ppc-tars-protocol/Common.h"
#include "ppc-tars-protocol/client/GatewayServiceClient.h"
#include "ppc-tars-service/FrontService/FrontServiceServer.h"

using namespace ppctars;
using namespace ppc::initializer;
using namespace ppc::rpc;
using namespace ppc::tools;
using namespace ppc::protocol;

void ProNodeServiceApp::destroyApp()
{
    NodeService_LOG(INFO) << LOG_DESC("destroyApp");
    if (m_rpc)
    {
        m_rpc->stop();
    }
    if (m_nodeInitializer)
    {
        m_nodeInitializer->stop();
    }
    NodeService_LOG(INFO) << LOG_DESC("destroyApp success");
}

void ProNodeServiceApp::initialize()
{
    try
    {
        m_configPath = tars::ServerConfig::BasePath + "/config.ini";
        addConfig("config.ini");

        NodeService_LOG(INFO) << LOG_DESC("initService") << LOG_KV("config", m_configPath);
        initService(m_configPath);
        NodeService_LOG(INFO) << LOG_DESC("initService success") << LOG_KV("config", m_configPath);
    }
    catch (std::exception const& e)
    {
        // since the tars will not print the detailed information when exceptioned, we print here
        std::cout << "init NodeServiceApp failed, error: " << boost::diagnostic_information(e)
                  << std::endl;
        throw e;
    }
}

void ProNodeServiceApp::initService(std::string const& _configPath)
{
    // init the log
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);
    auto ppcConfig = std::make_shared<PPCConfig>();
    ppcConfig->loadSelfTarsEndpoint(pt);
    // init the log
    m_logInitializer = std::make_shared<bcos::BoostLogInitializer>();
    m_logInitializer->initLog(pt);

    // init the nodeInitializer
    addConfig(std::string(NODE_PEM_NAME));
    NodeService_LOG(INFO) << LOG_DESC("initService: init the node");
    m_nodeInitializer = std::make_shared<Initializer>(_configPath);
    auto privateKeyPath = tars::ServerConfig::BasePath + "/" + std::string(NODE_PEM_NAME);
    NodeService_LOG(INFO) << LOG_DESC("generate the node private key path: ") << privateKeyPath;
    m_nodeInitializer->config()->setPrivateKeyPath(privateKeyPath);

    m_nodeInitializer->init(ppc::protocol::NodeArch::PRO);
    NodeService_LOG(INFO) << LOG_DESC("initService: init the node success");

    auto config = m_nodeInitializer->config();

    // init the gateway
    auto gatewayServiceName = config->gatewayServiceName();
    auto endPoints = config->getServiceEndPointsByName(gatewayServiceName);
    NodeService_LOG(INFO) << LOG_DESC("initGateway") << LOG_KV("serviceName", gatewayServiceName)
                          << LOG_KV("endPointSize", endPoints.size());
    auto gatewayPrx =
        createServantProxy<GatewayServicePrx>(true, gatewayServiceName, toTarsEndPoints(endPoints));
    auto gateway = std::make_shared<GatewayServiceClient>(
        gatewayServiceName, gatewayPrx, config->holdingMessageMinutes());
    NodeService_LOG(INFO) << LOG_DESC("initGateway success");

    // addservant for the front-service
    auto frontInitializer = m_nodeInitializer->frontInitializer();
    FrontServiceParam frontParam{frontInitializer};
    addServantWithParams<FrontServiceServer, FrontServiceParam>(
        getProxyDesc(ppc::protocol::FRONT_SERVANT_NAME), frontParam);
    // get the endpoint for front-service-object
    auto ret =
        getEndPointDescByAdapter(this, ppc::protocol::FRONT_SERVANT_NAME, ppcConfig->endpoint());
    if (!ret.first)
    {
        throw std::runtime_error("get load endpoint for front-service-object information failed");
    }

    std::string selfEndPoint = ret.second;
    NodeService_LOG(INFO) << LOG_DESC("get local-endpoint for the front-service-object success")
                          << LOG_KV("endPoint", selfEndPoint);
    frontInitializer->front()->setSelfEndPoint(selfEndPoint);
    // set the gateway into front
    m_nodeInitializer->frontInitializer()->front()->setGatewayInterface(gateway);

    // init the rpc
    NodeService_LOG(INFO) << LOG_DESC("init the rpc");
    // load the rpc config
    // not specify the certPath in air-mode
    config->loadRpcConfig(nullptr, pt);
    // init RpcStatusInterface
    RpcStatusInterface::Ptr rpcStatusInterface = std::make_shared<ppc::rpc::RpcMemory>(gateway);
    m_nodeInitializer->frontInitializer()->setRpcStatus(rpcStatusInterface);
    auto rpcFactory = std::make_shared<RpcFactory>(config->agencyID());
    m_rpc = rpcFactory->buildRpc(config);
    m_rpc->setRpcStorage(rpcStatusInterface);
    m_rpc->setBsEcdhPSI(m_nodeInitializer->bsEcdhPsi());

    m_nodeInitializer->registerRpcHandler(m_rpc);
    NodeService_LOG(INFO) << LOG_DESC("init the rpc success");

    // start the node
    NodeService_LOG(INFO) << LOG_DESC("start the node");
    m_nodeInitializer->start();
    NodeService_LOG(INFO) << LOG_DESC("start the node success");

    NodeService_LOG(INFO) << LOG_DESC("start the rpc");
    m_rpc->start();
    NodeService_LOG(INFO) << LOG_DESC("start the rpc success");
}