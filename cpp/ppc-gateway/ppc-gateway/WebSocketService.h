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
 * @file WebSocketService.h
 * @author: shawnhe
 * @date 2022-10-23
 */

#pragma once

#include "Common.h"
#include "GatewayConfigContext.h"
#include "ppc-http/src/Http.h"
#include "ppc-http/src/HttpClient.h"
#include "ppc-http/src/HttpFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <bcos-boostssl/websocket/WsService.h>
#include <bcos-utilities/Timer.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace ppc::gateway
{
class WebSocketService
{
public:
    using Ptr = std::shared_ptr<WebSocketService>;
    using WebSocketClientMap = std::unordered_map<std::string, bcos::boostssl::ws::WsService::Ptr>;
    using HttpClientMap = std::unordered_map<std::string, ppc::http::HttpClient::Ptr>;
    WebSocketService(
        GatewayConfigContext::Ptr _config, bcos::boostssl::ws::WsService::Ptr _wsServer)
      : m_config(_config),
        m_agencies(_config->config()->gatewayConfig().agencies),
        m_wsServer(std::move(_wsServer)),
        m_protocol(_config->config()->gatewayConfig().networkConfig.protocol),
        m_timer(std::make_shared<bcos::Timer>(
            _config->config()->gatewayConfig().reconnectTime, "connectTimer"))
    {}

    WebSocketService(GatewayConfigContext::Ptr _config, ppc::http::Http::Ptr _httpServer,
        std::shared_ptr<boost::asio::io_context> _ioContext)
      : m_config(_config),
        m_agencies(_config->config()->gatewayConfig().agencies),
        m_httpServer(std::move(_httpServer)),
        m_protocol(_config->config()->gatewayConfig().networkConfig.protocol),
        m_ioContext(_ioContext),
        m_timer(std::make_shared<bcos::Timer>(
            _config->config()->gatewayConfig().reconnectTime, "connectTimer"))
    {}

    virtual ~WebSocketService() = default;

    void start();
    void stop();

    void registerGatewayUrl(const std::string& _agencyID, const std::string& _agencyUrl);
    bool insertAgency(const std::string& _agencyID, const std::string& _agencyUrl);
    bcos::boostssl::ws::WsService::Ptr const& webSocketServer() const { return m_wsServer; }
    ppc::http::Http::Ptr httpServer() const { return m_httpServer; }
    bcos::boostssl::ws::WsService::Ptr webSocketClient(const std::string& _agencyID);
    ppc::http::HttpClient::Ptr httpClient(const std::string& _agencyID);
    GatewayConfigContext::Ptr const& gatewayConfig() const { return m_config; }


protected:
    virtual bcos::boostssl::ws::WsService::Ptr buildWebSocketClient(std::string const& _agencyID);
    virtual ppc::http::HttpClient::Ptr buildHttpClient(
        const std::string& _agencyID, std::shared_ptr<boost::asio::io_context> _ioContext);
    virtual void startConnect();
    virtual void reconnectUnconnectedClient();

    template <typename T, typename S>
    void insertIntoMap(std::string const& _key, T const& _value, bcos::SharedMutex& lock, S& _map)
    {
        bcos::WriteGuard l(lock);
        _map[_key] = _value;
    }

    template <typename T, typename S>
    T getValueFromMap(std::string const& _key, bcos::SharedMutex& lock, S const& _map)
    {
        bcos::ReadGuard l(lock);
        auto it = _map.find(_key);
        if (it != _map.end())
        {
            return it->second;
        }
        return nullptr;
    }

private:
    int m_protocol;
    GatewayConfigContext::Ptr m_config;
    bcos::SharedMutex x_agencies;
    std::map<std::string, std::vector<std::string>> m_agencies;
    std::map<std::string, std::string> m_urls;

    bcos::boostssl::ws::WsService::Ptr m_wsServer;
    ppc::http::Http::Ptr m_httpServer;
    // the timer used to try connecting to the un-connected-clients
    std::shared_ptr<bcos::Timer> m_timer;

    std::shared_ptr<boost::asio::io_context> m_ioContext;
    // key: agencyID, value: WebSocketClient
    WebSocketClientMap m_agencyClients;
    HttpClientMap m_agencyHttpClients;
    bcos::SharedMutex x_agencyClients;
    // connect failed for all the agency-nodes are offline
    std::set<std::string> m_unConnectedAgencies;
    mutable bcos::SharedMutex x_unConnectedAgencies;
};


class WebSocketServiceFactory
{
public:
    using Ptr = std::shared_ptr<WebSocketServiceFactory>;

public:
    WebSocketServiceFactory() = default;
    ~WebSocketServiceFactory() = default;

    WebSocketService::Ptr buildWebSocketService(const ppc::tools::PPCConfig::Ptr& _config,
        std::shared_ptr<boost::asio::io_context> _ioContext = nullptr);

private:
    bcos::boostssl::ws::WsService::Ptr buildWebSocketServer(
        const GatewayConfigContext::Ptr& _config);

    ppc::http::Http::Ptr buildHttpServer(const GatewayConfigContext::Ptr& _config);
};

}  // namespace ppc::gateway