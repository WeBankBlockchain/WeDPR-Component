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
 * @file HttpFactory.cpp
 * @author: zachma
 * @date 2023-8-23
 */

#include "HttpFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <bcos-boostssl/websocket/WsInitializer.h>
#include <bcos-boostssl/websocket/WsService.h>

using namespace bcos;
using namespace ppc::http;
using namespace ppc::tools;

Http::Ptr HttpFactory::buildHttp(ppc::tools::PPCConfig::ConstPtr _config)
{
    auto wsConfig = initConfig(_config);
    // create the wsConfig
    auto wsService = std::make_shared<bcos::boostssl::ws::WsService>();
    wsService->setTimerFactory(std::make_shared<timer::TimerFactory>());
    auto initializer = std::make_shared<bcos::boostssl::ws::WsInitializer>();

    initializer->setConfig(wsConfig);
    initializer->initWsService(wsService);

    return std::make_shared<Http>(
        wsService, m_selfPartyID, _config->gatewayConfig().networkConfig.token);
}


std::shared_ptr<boostssl::ws::WsConfig> HttpFactory::initConfig(
    ppc::tools::PPCConfig::ConstPtr _config)
{
    // init the wsConfig
    auto wsConfig = std::make_shared<boostssl::ws::WsConfig>();
    wsConfig->setModel(boostssl::ws::WsModel::Server);

    wsConfig->setListenIP(_config->gatewayConfig().networkConfig.listenIp);
    wsConfig->setListenPort(_config->gatewayConfig().networkConfig.listenPort);
    wsConfig->setThreadPoolSize(_config->gatewayConfig().networkConfig.threadPoolSize);
    wsConfig->setDisableSsl(_config->gatewayConfig().networkConfig.disableSsl);
    if (_config->gatewayConfig().networkConfig.disableSsl)
    {
        return wsConfig;
    }
    // non-sm-ssl
    auto contextConfig = std::make_shared<boostssl::context::ContextConfig>();
    if (!_config->gatewayConfig().networkConfig.enableSM)
    {
        boostssl::context::ContextConfig::CertConfig certConfig;
        certConfig.caCert = _config->gatewayConfig().networkConfig.caCertPath;
        certConfig.nodeCert = _config->gatewayConfig().networkConfig.sslCertPath;
        certConfig.nodeKey = _config->gatewayConfig().networkConfig.sslKeyPath;
        contextConfig->setCertConfig(certConfig);
        contextConfig->setSslType("ssl");
        HTTP_LOG(INFO) << LOG_DESC("initConfig: http work in non-sm-ssl model")
                       << LOG_KV("caCert", certConfig.caCert)
                       << LOG_KV("nodeCert", certConfig.nodeCert)
                       << LOG_KV("nodeKey", certConfig.nodeKey);
        wsConfig->setContextConfig(contextConfig);
        return wsConfig;
    }
    // sm-ssl
    boostssl::context::ContextConfig::SMCertConfig certConfig;
    certConfig.caCert = _config->gatewayConfig().networkConfig.smCaCertPath;
    certConfig.nodeCert = _config->gatewayConfig().networkConfig.smSslCertPath;
    certConfig.nodeKey = _config->gatewayConfig().networkConfig.smSslKeyPath;
    certConfig.enNodeCert = _config->gatewayConfig().networkConfig.smEnSslCertPath;
    certConfig.enNodeKey = _config->gatewayConfig().networkConfig.smEnSslKeyPath;
    contextConfig->setSmCertConfig(certConfig);
    contextConfig->setSslType("sm_ssl");

    BCOS_LOG(INFO) << LOG_DESC("initConfig") << LOG_DESC("http work in sm ssl model")
                   << LOG_KV("caCert", certConfig.caCert) << LOG_KV("nodeCert", certConfig.nodeCert)
                   << LOG_KV("enNodeCert", certConfig.enNodeCert);

    wsConfig->setContextConfig(contextConfig);
    return wsConfig;
}
