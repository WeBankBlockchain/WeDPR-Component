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
 * @file GatewayConfigContext.cpp
 * @author: shawnhe
 * @date 2022-10-23
 */

#include "GatewayConfigContext.h"

using namespace bcos;
using namespace ppc::gateway;
void GatewayConfigContext::initContextConfig()
{
    m_contextConfig = std::make_shared<bcos::boostssl::context::ContextConfig>();
    auto const& gatewayConfig = m_config->gatewayConfig().networkConfig;
    // non-sm-ssl
    if (!gatewayConfig.enableSM)
    {
        boostssl::context::ContextConfig::CertConfig certConfig;
        certConfig.caCert = gatewayConfig.caCertPath;
        certConfig.nodeCert = gatewayConfig.sslCertPath;
        certConfig.nodeKey = gatewayConfig.sslKeyPath;
        m_contextConfig->setCertConfig(certConfig);
        m_contextConfig->setSslType("ssl");
        GATEWAY_LOG(INFO) << LOG_DESC("initConfig: rpc work in non-sm-ssl model")
                          << LOG_KV("caCert", certConfig.caCert)
                          << LOG_KV("nodeCert", certConfig.nodeCert)
                          << LOG_KV("nodeKey", certConfig.nodeKey);
        GATEWAY_LOG(INFO) << LOG_DESC("initContextConfig: non-sm-ssl");
        return;
    }
    // sm-ssl
    boostssl::context::ContextConfig::SMCertConfig certConfig;
    certConfig.caCert = gatewayConfig.smCaCertPath;
    certConfig.nodeCert = gatewayConfig.smSslCertPath;
    certConfig.nodeKey = gatewayConfig.smSslKeyPath;
    certConfig.enNodeCert = gatewayConfig.smEnSslCertPath;
    certConfig.enNodeKey = gatewayConfig.smEnSslKeyPath;
    m_contextConfig->setSmCertConfig(certConfig);
    m_contextConfig->setSslType("sm_ssl");
    GATEWAY_LOG(INFO) << LOG_DESC("initContextConfig: sm-ssl");
}
