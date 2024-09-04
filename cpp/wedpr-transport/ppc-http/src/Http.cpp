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
 * @file Http.cpp
 * @author: zachma
 * @date 2023-8-23
 */

#include "Http.h"
#include "ppc-framework/http/HttpTypeDef.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-tools/src/config/ParamChecker.h"
#include <bcos-utilities/Error.h>

using namespace bcos;
using namespace ppc::http;
using namespace ppc::tools;
using namespace ppc::protocol;

Http::Http(std::shared_ptr<boostssl::ws::WsService> _wsService, std::string const& _selfPartyID,
    std::string const& _token)
  : m_wsService(std::move(_wsService)), m_token(_token)
{
    // register handler for http
    auto httpServer = m_wsService->httpServer();
    if (httpServer)
    {
        httpServer->setHttpReqHandler([this](bcos::boostssl::http::HttpRequest&& _request,
                                          std::function<void(bcos::bytes)> _responseHandler) {
            this->onHttpRequest(std::move(_request), _responseHandler);
        });
    }
    HTTP_LOG(INFO) << LOG_DESC("init http success") << LOG_KV("selfParty", _selfPartyID);
}

void Http::onHttpRequest(
    bcos::boostssl::http::HttpRequest&& _request, std::function<void(bcos::bytes)> _responseHandler)
{
    std::string uri;
    std::map<std::string, std::string> params;
    std::string url = std::string(_request.target().data(), _request.target().size());
    parseURL(url, uri, params);
    auto it = m_urlToHandler.find(uri);
    if (it == m_urlToHandler.end())
    {
        HTTP_LOG(DEBUG) << LOG_DESC("onHttpRequest: uri not found") << LOG_KV("uri", uri);
        BOOST_THROW_EXCEPTION(BCOS_ERROR(
            (int64_t)HttpError::UriNotFound, "The uri does not exist/is not available."));
    }
    auto const& methodHandler = it->second;
    HTTP_LOG(DEBUG) << LOG_DESC("onHttpRequest: handle uri") << LOG_KV("uri", uri);
    methodHandler(std::move(_request), [_responseHandler](Error::Ptr _error,
                                           bcos::bytes&& _result) { _responseHandler(_result); });
}