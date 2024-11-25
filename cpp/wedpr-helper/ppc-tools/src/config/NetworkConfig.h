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
 * @file NetworkConfig.h
 * @author: yujiechen
 * @date 2022-11-4
 */
#pragma once

namespace ppc::tools
{
struct NetworkConfig
{
    constexpr static int DefaultRpcListenPort = 10200;

    // the configuration for rpc
    std::string listenIp;
    int listenPort;
    uint32_t threadPoolSize;
    std::string token;
    bool disableSsl;

    bool enableSM;
    // the gateway protocol
    int protocol;  // websocket or http
    std::string url;
    // the rpc cert path
    std::string certPath;
    // the cert info for non-sm
    std::string caCertPath;
    std::string sslKeyPath;
    std::string sslCertPath;
    // the cert info for sm
    std::string smCaCertPath;
    std::string smSslKeyPath;
    std::string smSslCertPath;
    std::string smEnSslKeyPath;
    std::string smEnSslCertPath;

    constexpr static std::string_view CA_CERT_NAME = "ca.crt";
    constexpr static std::string_view SM_CA_CERT_NAME = "sm_ca.crt";

    constexpr static std::string_view SSL_CERT_NAME = "ssl.crt";
    constexpr static std::string_view SM_SSL_CERT_NAME = "sm_ssl.crt";

    constexpr static std::string_view SSL_KEY_NAME = "ssl.key";
    constexpr static std::string_view SM_SSL_KEY_NAME = "sm_ssl.key";

    constexpr static std::string_view SM_SSL_EN_CERT_NAME = "sm_enssl.crt";
    constexpr static std::string_view SM_SSL_EN_KEY_NAME = "sm_enssl.key";

    constexpr static int PROTOCOL_WEBSOCKET = 0;
    constexpr static int PROTOCOL_HTTP = 1;
};
}  // namespace ppc::tools