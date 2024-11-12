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
 * @file http_demo.cpp
 * @author: zachma
 * @date 2023-8-23
 */

#include "libhelper/CommandHelper.h"
#include "libhelper/ExitHandler.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-http/src/HttpFactory.h"
#include <bcos-utilities/Common.h>

using namespace bcos;
using namespace ppc;
using namespace ppc::http;
using namespace ppc::tools;

void registerUrlHandler(const std::string& _url, HttpInterface::Ptr _http)
{
    _http->registerUrlHandler(
        _url, [](bcos::boostssl::http::HttpRequest const& _httpReq, RespUrlFunc _handler) {
            auto reqs = _httpReq.body();
            std::cout << " [Url] ===>>>> " << LOG_KV("url_request", reqs) << std::endl;
            _handler(nullptr, std::move(bcos::bytes(reqs.begin(), reqs.end())));
        });
}

int main(int argc, char const* argv[])
{
    ExitHandler exitHandler;
    signal(SIGTERM, &ExitHandler::exitHandler);
    signal(SIGABRT, &ExitHandler::exitHandler);
    signal(SIGINT, &ExitHandler::exitHandler);

    try
    {
        auto param = initCommandLine("http_demo", argc, argv);
        auto ppcConfig = std::make_shared<PPCConfig>();
        // not specify the certPath in air-mode
        ppcConfig->loadGatewayConfig(param.configFilePath);
        auto httpFactory = std::make_shared<HttpFactory>("selfParty");
        auto url = ppcConfig->gatewayConfig().networkConfig.url;
        auto http = httpFactory->buildHttp(ppcConfig);
        registerUrlHandler(url, http);
        // start the http
        http->start();
        while (!exitHandler.shouldExit())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "[" << bcos::getCurrentDateTime() << "] ";
        std::cout << "http-demo program exit normally." << std::endl;
    }
    catch (std::exception const& e)
    {
        ppc::printVersion("http-demo");
        std::cout << "[" << bcos::getCurrentDateTime() << "] ";
        std::cout << "start http-demo failed, error:" << boost::diagnostic_information(e)
                  << std::endl;
        return -1;
    }
    return 0;
}
