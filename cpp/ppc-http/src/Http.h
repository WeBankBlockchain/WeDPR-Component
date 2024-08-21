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
 * @file Http.h
 * @author: zachma
 * @date 2023-8-23
 */
#pragma once
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/http/HttpInterface.h"
#include <bcos-boostssl/httpserver/Common.h>
#include <bcos-boostssl/interfaces/MessageFace.h>
#include <bcos-boostssl/websocket/WsService.h>
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::http
{
class Http : public HttpInterface
{
public:
    using Ptr = std::shared_ptr<Http>;
    Http(std::shared_ptr<bcos::boostssl::ws::WsService> _wsService, std::string const& _selfPartyID,
        std::string const& _token);
    ~Http() override { stop(); }

    void start() override
    {
        if (m_wsService)
        {
            m_wsService->start();
        }
        HTTP_LOG(INFO) << LOG_DESC("start HTTP");
    }

    void stop() override
    {
        if (m_wsService)
        {
            m_wsService->stop();
        }
        HTTP_LOG(INFO) << LOG_DESC("stop HTTP");
    }

    void registerUrlHandler(std::string const& _urlName,
        std::function<void(bcos::boostssl::http::HttpRequest const&, RespUrlFunc)> _handler)
        override
    {
        bcos::UpgradableGuard l(x_urlToHandler);
        if (m_urlToHandler.count(_urlName))
        {
            HTTP_LOG(INFO) << LOG_DESC("registerUrlHandler return for url handler already exists")
                           << LOG_KV("url", _urlName);
            return;
        }
        bcos::UpgradeGuard ul(l);
        m_urlToHandler[_urlName] = _handler;
        HTTP_LOG(INFO) << LOG_DESC("registerUrlHandler success") << LOG_KV("url", _urlName);
    }

protected:
    virtual void onHttpRequest(bcos::boostssl::http::HttpRequest&& _request,
        std::function<void(bcos::bytes)> _responseHandler);

    void parseURL(
        const std::string& _url, std::string& uri, std::map<std::string, std::string>& params_map)
    {
        std::string params;
        std::string url;
        auto paramsStart = _url.find("?");
        if (paramsStart != std::string::npos)
        {
            params = _url.substr(paramsStart + 1);
            uri = _url.substr(0, paramsStart - 1);
        }
        else
        {
            uri = _url;
        }

        auto pos = 0U;
        while (pos < params.size())
        {
            auto eqPos = params.find("=", pos);
            auto ampPos = params.find("&", pos);

            std::string paramName = params.substr(pos, eqPos - pos);
            std::string paramValue;
            if (ampPos != std::string::npos)
            {
                paramValue = params.substr(eqPos + 1, ampPos - eqPos - 1);
            }
            else
            {
                paramValue = params.substr(eqPos + 1);
            }

            params_map[paramName] = paramValue;
            if (ampPos == std::string::npos)
            {
                break;
            }

            pos = ampPos + 1;
        }
    }

private:
    std::shared_ptr<bcos::boostssl::ws::WsService> m_wsService;

    // the url_name to function
    std::map<std::string,
        std::function<void(bcos::boostssl::http::HttpRequest const&, RespUrlFunc)>>
        m_urlToHandler;

    bcos::SharedMutex x_urlToHandler;

    std::string m_token;
};
}  // namespace ppc::http
