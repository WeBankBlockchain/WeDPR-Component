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
 * @file RpcFactory.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "RpcFactory.h"
#include "ppc-storage/src/mysql/MySQLStorage.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <bcos-boostssl/websocket/WsInitializer.h>
#include <bcos-boostssl/websocket/WsService.h>

using namespace bcos;
using namespace ppc::rpc;
using namespace ppc::tools;

Rpc::Ptr RpcFactory::buildRpc(
    ppc::tools::PPCConfig::ConstPtr _config, ppc::gateway::IGateway::Ptr gateway)
{
    auto wsConfig = initConfig(_config);
    // create the wsConfig
    auto wsService = std::make_shared<bcos::boostssl::ws::WsService>("WeDPR-RPC");
    wsService->setTimerFactory(std::make_shared<timer::TimerFactory>());
    auto initializer = std::make_shared<bcos::boostssl::ws::WsInitializer>();

    initializer->setConfig(wsConfig);
    initializer->initWsService(wsService);

    auto rpc = std::make_shared<Rpc>(
        wsService, gateway, m_selfPartyID, _config->rpcConfig().token, _config->dataLocation());
    return rpc;
}

std::shared_ptr<boostssl::ws::WsConfig> RpcFactory::initConfig(
    ppc::tools::PPCConfig::ConstPtr _config)
{
    // init the wsConfig
    auto wsConfig = std::make_shared<boostssl::ws::WsConfig>();
    wsConfig->setModel(boostssl::ws::WsModel::Server);

    wsConfig->setListenIP(_config->rpcConfig().listenIp);
    wsConfig->setListenPort(_config->rpcConfig().listenPort);
    wsConfig->setThreadPoolSize(_config->rpcConfig().threadPoolSize);
    wsConfig->setDisableSsl(_config->rpcConfig().disableSsl);
    if (_config->rpcConfig().disableSsl)
    {
        return wsConfig;
    }
    // non-sm-ssl
    auto contextConfig = std::make_shared<boostssl::context::ContextConfig>();
    if (!_config->rpcConfig().enableSM)
    {
        boostssl::context::ContextConfig::CertConfig certConfig;
        certConfig.caCert = _config->rpcConfig().caCertPath;
        certConfig.nodeCert = _config->rpcConfig().sslCertPath;
        certConfig.nodeKey = _config->rpcConfig().sslKeyPath;
        contextConfig->setCertConfig(certConfig);
        contextConfig->setSslType("ssl");
        RPC_LOG(INFO) << LOG_DESC("initConfig: rpc work in non-sm-ssl model")
                      << LOG_KV("caCert", certConfig.caCert)
                      << LOG_KV("nodeCert", certConfig.nodeCert)
                      << LOG_KV("nodeKey", certConfig.nodeKey);
        wsConfig->setContextConfig(contextConfig);
        return wsConfig;
    }
    // sm-ssl
    boostssl::context::ContextConfig::SMCertConfig certConfig;
    certConfig.caCert = _config->rpcConfig().smCaCertPath;
    certConfig.nodeCert = _config->rpcConfig().smSslCertPath;
    certConfig.nodeKey = _config->rpcConfig().smSslKeyPath;
    certConfig.enNodeCert = _config->rpcConfig().smEnSslCertPath;
    certConfig.enNodeKey = _config->rpcConfig().smEnSslKeyPath;
    contextConfig->setSmCertConfig(certConfig);
    contextConfig->setSslType("sm_ssl");

    BCOS_LOG(INFO) << LOG_DESC("initConfig") << LOG_DESC("rpc work in sm ssl model")
                   << LOG_KV("caCert", certConfig.caCert) << LOG_KV("nodeCert", certConfig.nodeCert)
                   << LOG_KV("enNodeCert", certConfig.enNodeCert);

    wsConfig->setContextConfig(contextConfig);
    return wsConfig;
}