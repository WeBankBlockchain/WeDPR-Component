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
 * @file WebSocketService.cpp
 * @author: shawnhe
 * @date 2022-10-23
 */

#include "WebSocketService.h"
#include "ppc-tools/src/config/ParamChecker.h"
#include <bcos-boostssl/websocket/WsInitializer.h>

using namespace bcos;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;
using namespace ppc::gateway;
using namespace ppc::tools;
using namespace ppc::http;

void WebSocketService::start()
{
    if (m_timer)
    {
        m_timer->registerTimeoutHandler(
            boost::bind(&WebSocketService::reconnectUnconnectedClient, this));
    }
    if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
    {
        m_wsServer->start();
        GATEWAY_LOG(INFO) << LOG_BADGE("start the WebSocketService end");
    }
    else if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_HTTP)
    {
        m_httpServer->start();
        GATEWAY_LOG(INFO) << LOG_BADGE("start the HttpService end");
    }
    startConnect();
    m_timer->start();
}

void WebSocketService::stop()
{
    GATEWAY_LOG(INFO) << LOG_BADGE("stop the WebSocketService");
    if (m_timer)
    {
        m_timer->stop();
    }
    ReadGuard l(x_agencyClients);
    if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
    {
        for (auto& client : m_agencyClients)
        {
            auto it = client.second;
            if (it)
            {
                it->stop();
            }
        }
        if (m_wsServer)
        {
            m_wsServer->stop();
        }
        GATEWAY_LOG(INFO) << LOG_BADGE("stop the WebSocketService success");
    }
    else if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_HTTP)
    {
        if (m_httpServer)
        {
            m_httpServer->stop();
        }
        GATEWAY_LOG(INFO) << LOG_BADGE("stop the HttpService success");
    }
}


void WebSocketService::registerGatewayUrl(
    const std::string& _agencyID, const std::string& _agencyUrl)
{
    if (!insertAgency(_agencyID, _agencyUrl))
    {
        return;
    }
    GATEWAY_LOG(INFO) << LOG_BADGE("registerGatewayUrl") << LOG_KV("agencyID", _agencyID)
                      << LOG_KV("agencyUrl", _agencyUrl);
    try
    {
        if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
        {
            auto client = buildWebSocketClient(_agencyID);
            insertIntoMap(_agencyID, client, x_agencyClients, m_agencyClients);
        }
        else if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_HTTP)
        {
            auto client = buildHttpClient(_agencyID, m_ioContext);
            insertIntoMap(_agencyID, client, x_agencyClients, m_agencyHttpClients);
        }
    }
    catch (std::exception const& e)
    {
        GATEWAY_LOG(WARNING) << LOG_DESC("connect to the agency failed")
                             << LOG_KV("agency", _agencyID) << LOG_KV("url", _agencyUrl);
        {
            bcos::WriteGuard ucl(x_unConnectedAgencies);
            m_unConnectedAgencies.insert(_agencyID);
        }
    }
}

bool WebSocketService::insertAgency(const std::string& _agencyID, const std::string& _agencyUrl)
{
    std::vector<std::string> endpoints;
    boost::split(endpoints, _agencyUrl, boost::is_any_of(","));

    WriteGuard l(x_agencies);

    if (m_urls.find(_agencyID) != m_urls.end())
    {
        if (m_urls[_agencyID] == _agencyUrl)
        {
            // no need update
            return false;
        }
    }

    m_urls[_agencyID] = _agencyUrl;
    m_agencies[_agencyID] = endpoints;
    return true;
}

void WebSocketService::reconnectUnconnectedClient()
{
    {
        // print connected clients
        bcos::ReadGuard rl(x_agencyClients);
        if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
        {
            GATEWAY_LOG(INFO) << LOG_DESC("connectedWebsocketClient")
                              << LOG_KV("size", m_agencyClients.size());
        }
        else if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_HTTP)
        {
            GATEWAY_LOG(INFO) << LOG_DESC("connectedHttpClient")
                              << LOG_KV("size", m_agencyHttpClients.size());
        }
    }

    // start reconnecting
    bcos::UpgradableGuard l(x_unConnectedAgencies);
    GATEWAY_LOG(INFO) << LOG_DESC("reconnectUnconnectedClient")
                      << LOG_KV("size", m_unConnectedAgencies.size());
    for (auto it = m_unConnectedAgencies.begin(); it != m_unConnectedAgencies.end();)
    {
        try
        {
            if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
            {
                GATEWAY_LOG(INFO) << LOG_BADGE("WebSocketService")
                                  << LOG_DESC("reconnectUnconnectedClient: connect to peer")
                                  << LOG_KV("agency", *it);
                auto client = buildWebSocketClient(*it);
                GATEWAY_LOG(INFO) << LOG_BADGE("WebSocketService")
                                  << LOG_DESC("reconnectUnconnectedClient: connect to peer success")
                                  << LOG_KV("agency", *it);
                // insert the successfully started client into the m_agencyClients
                insertIntoMap(*it, client, x_agencyClients, m_agencyClients);
                // erase the connected client from the m_unConnectedAgencies
                bcos::UpgradeGuard ul(l);
                it = m_unConnectedAgencies.erase(it);
            }
            else if (m_protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_HTTP)
            {
                GATEWAY_LOG(INFO) << LOG_BADGE("HttpService")
                                  << LOG_DESC("reconnectUnconnectedClient: connect to peer")
                                  << LOG_KV("agency", *it);
                auto client = buildHttpClient(*it, m_ioContext);
                GATEWAY_LOG(INFO) << LOG_BADGE("HttpService")
                                  << LOG_DESC("reconnectUnconnectedClient: connect to peer success")
                                  << LOG_KV("agency", *it);
                // insert the successfully started client into the m_agencyHttpClients
                insertIntoMap(*it, client, x_agencyClients, m_agencyHttpClients);
                // erase the connected client from the m_unConnectedAgencies
                bcos::UpgradeGuard ul(l);
                it = m_unConnectedAgencies.erase(it);
            }
        }
        catch (std::exception const& e)
        {
            it++;
            GATEWAY_LOG(INFO) << LOG_BADGE("reconnectUnconnectedClient failed");
        }
    }
    m_timer->restart();
}

WsService::Ptr WebSocketService::webSocketClient(const std::string& _agencyID)
{
    return getValueFromMap<WsService::Ptr, WebSocketClientMap>(
        _agencyID, x_agencyClients, m_agencyClients);
}

HttpClient::Ptr WebSocketService::httpClient(const std::string& _agencyID)
{
    return getValueFromMap<HttpClient::Ptr, HttpClientMap>(
        _agencyID, x_agencyClients, m_agencyHttpClients);
}

void WebSocketService::startConnect()
{
    GATEWAY_LOG(INFO) << LOG_DESC("WebSocketService: startConnect");
    auto const& agencyConfig = m_config->config()->gatewayConfig().agencies;
    auto protocol = m_config->config()->gatewayConfig().networkConfig.protocol;
    for (auto const& it : agencyConfig)
    {
        try
        {
            if (protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
            {
                auto client = buildWebSocketClient(it.first);
                insertIntoMap(it.first, client, x_agencyClients, m_agencyClients);
            }
            else if (protocol == m_config->config()->gatewayConfig().networkConfig.PROTOCOL_HTTP)
            {
                auto client = buildHttpClient(it.first, m_ioContext);
                insertIntoMap(it.first, client, x_agencyClients, m_agencyHttpClients);
            }
        }
        catch (std::exception const& e)
        {
            {
                bcos::WriteGuard l(x_unConnectedAgencies);
                m_unConnectedAgencies.insert(it.first);
            }
            GATEWAY_LOG(WARNING) << LOG_BADGE("startConnect")
                                 << LOG_DESC("connect to the agency failed")
                                 << LOG_KV("agency", it.first)
                                 << LOG_KV("exception", boost::diagnostic_information(e));
        }
    }
    GATEWAY_LOG(INFO) << LOG_DESC("WebSocketService: startConnect success");
}

HttpClient::Ptr WebSocketService::buildHttpClient(
    const std::string& _agencyID, std::shared_ptr<boost::asio::io_context> _ioContext)
{
    GATEWAY_LOG(INFO) << LOG_BADGE("WebSocketService: buildHttpClient")
                      << LOG_KV("agency", _agencyID);
    {
        bcos::ReadGuard l(x_agencies);
        // one agencyID => one httpClient
        for (const auto& endpoint : m_agencies.at(_agencyID))
        {
            if (!checkEndpoint(endpoint))
            {
                BOOST_THROW_EXCEPTION(
                    InvalidParameter() << bcos::errinfo_comment("Invalid endpoint: " + endpoint));
            }
            std::vector<std::string> url;
            boost::split(url, endpoint, boost::is_any_of(":"), boost::token_compress_on);
            auto client = std::make_shared<HttpClient>(*_ioContext, url[0], std::stoi(url[1]));
            return client;
        }
    }
}

WsService::Ptr WebSocketService::buildWebSocketClient(std::string const& _agencyID)
{
    GATEWAY_LOG(INFO) << LOG_BADGE("buildWebSocketClient") << LOG_KV("agency", _agencyID);
    auto peers = std::make_shared<EndPoints>();
    {
        bcos::ReadGuard l(x_agencies);
        for (const auto& endpoint : m_agencies.at(_agencyID))
        {
            if (!checkEndpoint(endpoint))
            {
                BOOST_THROW_EXCEPTION(
                    InvalidParameter() << bcos::errinfo_comment("Invalid endpoint: " + endpoint));
            }

            std::vector<std::string> url;
            boost::split(url, endpoint, boost::is_any_of(":"), boost::token_compress_on);
            NodeIPEndpoint nodeIpEndpoint = NodeIPEndpoint(url[0], std::stoi(url[1]));
            peers->insert(nodeIpEndpoint);
        }
    }

    auto const& gatewayConfig = m_config->config()->gatewayConfig();
    auto wsConfig = std::make_shared<WsConfig>();
    wsConfig->setModel(WsModel::Client);
    wsConfig->setConnectPeers(peers);
    wsConfig->setThreadPoolSize(gatewayConfig.networkConfig.threadPoolSize);
    wsConfig->setDisableSsl(gatewayConfig.networkConfig.disableSsl);
    wsConfig->setMaxMsgSize(gatewayConfig.maxAllowedMsgSize);
    if (!wsConfig->disableSsl())
    {
        wsConfig->setContextConfig(m_config->contextConfig());
    }

    auto wsInitializer = std::make_shared<WsInitializer>();
    wsInitializer->setConfig(wsConfig);
    auto wsClient = std::make_shared<WsService>(GATEWAY_WS_CLIENT_MODULE);
    wsClient->setTimerFactory(std::make_shared<timer::TimerFactory>());
    wsInitializer->initWsService(wsClient);

    wsClient->start();
    GATEWAY_LOG(INFO) << LOG_BADGE("WebSocketService") << LOG_DESC("connect to peer success")
                      << LOG_KV("agency", _agencyID);
    return wsClient;
}

WsService::Ptr WebSocketServiceFactory::buildWebSocketServer(
    const GatewayConfigContext::Ptr& _config)
{
    GATEWAY_LOG(INFO) << LOG_BADGE("buildWebSocketServer");
    auto wsConfig = std::make_shared<WsConfig>();
    wsConfig->setModel(WsModel::Server);

    auto const& gatewayConfig = _config->config()->gatewayConfig();
    wsConfig->setListenIP(gatewayConfig.networkConfig.listenIp);
    wsConfig->setListenPort(gatewayConfig.networkConfig.listenPort);
    wsConfig->setThreadPoolSize(gatewayConfig.networkConfig.threadPoolSize);
    wsConfig->setDisableSsl(gatewayConfig.networkConfig.disableSsl);
    if (!wsConfig->disableSsl())
    {
        wsConfig->setContextConfig(_config->contextConfig());
    }
    wsConfig->setMaxMsgSize(gatewayConfig.maxAllowedMsgSize);
    auto wsInitializer = std::make_shared<WsInitializer>();
    wsInitializer->setConfig(wsConfig);
    auto wsService = std::make_shared<WsService>(GATEWAY_WS_SERVER_MODULE);
    wsService->setTimerFactory(std::make_shared<timer::TimerFactory>());
    wsInitializer->initWsService(wsService);

    return wsService;
}

Http::Ptr WebSocketServiceFactory::buildHttpServer(const GatewayConfigContext::Ptr& _config)
{
    GATEWAY_LOG(INFO) << LOG_BADGE("buildHttpServer");
    auto ppcConfig = _config->config();
    auto httpFactory = std::make_shared<HttpFactory>(ppcConfig->agencyID());
    return httpFactory->buildHttp(ppcConfig);
}

WebSocketService::Ptr WebSocketServiceFactory::buildWebSocketService(
    ppc::tools::PPCConfig::Ptr const& _config, std::shared_ptr<boost::asio::io_context> _ioContext)
{
    try
    {
        auto gatewayConfig = std::make_shared<GatewayConfigContext>(_config);
        auto _protocol = _config->gatewayConfig().networkConfig.protocol;
        // init websocket service
        if (_protocol == _config->gatewayConfig().networkConfig.PROTOCOL_WEBSOCKET)
        {
            GATEWAY_LOG(INFO) << LOG_BADGE("buildWebSocketService");
            auto wsServer = buildWebSocketServer(gatewayConfig);
            auto webSocketService = std::make_shared<WebSocketService>(gatewayConfig, wsServer);
            return webSocketService;
        }
        else if (_protocol == _config->gatewayConfig().networkConfig.PROTOCOL_HTTP)
        {
            GATEWAY_LOG(INFO) << LOG_BADGE("buildHttpService");
            auto httpServer = buildHttpServer(gatewayConfig);
            auto webSocketService =
                std::make_shared<WebSocketService>(gatewayConfig, httpServer, _ioContext);
            return webSocketService;
        }
    }
    catch (std::exception const& e)
    {
        GATEWAY_LOG(ERROR) << LOG_BADGE("buildWebSocketService")
                           << LOG_DESC("init gateway websocket service failed, error: " +
                                       boost::diagnostic_information(e));
        throw e;
    }
}
