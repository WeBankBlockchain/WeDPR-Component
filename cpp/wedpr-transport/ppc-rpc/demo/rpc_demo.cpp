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
 * @file rpc_demo.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "libhelper/CommandHelper.h"
#include "libhelper/ExitHandler.h"
#include "ppc-rpc/src/RpcFactory.h"
#include <bcos-utilities/Common.h>

using namespace bcos;
using namespace ppc;
using namespace ppc::rpc;
using namespace ppc::tools;

void registerEchoHandler(RpcInterface::Ptr _rpc)
{
    _rpc->registerHandler("echo", [](Json::Value const& _request, RespFunc _func) {
        Json::Value response;
        response["request"] = _request;
        response["tag"] = "echo";
        _func(nullptr, std::move(response));
    });
}

int main(int argc, const char* argv[])
{
    ExitHandler exitHandler;
    signal(SIGTERM, &ExitHandler::exitHandler);
    signal(SIGABRT, &ExitHandler::exitHandler);
    signal(SIGINT, &ExitHandler::exitHandler);
    try
    {
        auto param = initCommandLine("rpc_demo", argc, argv);
        auto ppcConfig = std::make_shared<PPCConfig>();
        // not specify the certPath in air-mode
        ppcConfig->loadRpcConfig(param.configFilePath);
        auto rpcFactory = std::make_shared<RpcFactory>("selfParty");
        auto rpc = rpcFactory->buildRpc(ppcConfig, nullptr);
        registerEchoHandler(rpc);
        // start the rpc
        rpc->start();
        while (!exitHandler.shouldExit())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "[" << bcos::getCurrentDateTime() << "] ";
        std::cout << "rpc-demo program exit normally." << std::endl;
    }
    catch (std::exception const& e)
    {
        ppc::printVersion("rpc_demo");
        std::cout << "[" << bcos::getCurrentDateTime() << "] ";
        std::cout << "start rpc-demo failed, error:" << boost::diagnostic_information(e)
                  << std::endl;
        return -1;
    }
}