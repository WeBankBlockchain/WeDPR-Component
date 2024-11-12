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
 * @file gateway_demo.cpp
 * @author: shawnhe
 * @date 2022-10-28
 */
#if 0
#include "ppc-gateway/ppc-gateway/Gateway.h"
#include "ppc-gateway/ppc-gateway/GatewayConfigContext.h"
#include "ppc-tools/src/config/PPCConfig.h"

using namespace ppc;
using namespace ppctars;
using namespace ppc::front;
using namespace ppc::gateway;
using namespace ppc::storage;
using namespace ppc::protocol;

using namespace bcos;
using namespace ppc::tools;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;


inline static PPCMessageFace::Ptr buildMessage(std::string _taskID, uint32_t _seq)
{
    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto message = messageFactory->buildPPCMessage();
    message->setVersion(1);
    message->setTaskType(uint8_t(protocol::TaskType::PSI));
    message->setAlgorithmType(uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC));
    message->setMessageType(4);
    message->setSeq(_seq);
    message->setTaskID(_taskID);
    message->setSender("1001");
    message->setData(std::make_shared<bcos::bytes>(10, 'a'));
    return message;
}


int main(int argc, char* argv[])
{
    // config
    auto config = std::make_shared<PPCConfig>();
    // not specify the certPath in air-mode
    config->(nullptr, "../../../../ppc-gateway/test/data/config0.ini");
    auto gatewayConfig = config->gatewayConfig();

    // global thread pool
    auto threadPoolSize = gatewayConfig.networkConfig.threadPoolSize;
    auto threadPool = std::make_shared<ThreadPool>(GATEWAY_THREAD_POOL_MODULE, threadPoolSize);

    auto ioService = std::make_shared<boost::asio::io_service>();

    // front factory
    auto frontFactory = std::make_shared<FrontFactory>("1001", threadPool);
    auto front = frontFactory->buildFront(ioService);

    // message factory
    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto wsMessageFactory = std::make_shared<WsMessageFactory>();

    // Note: no-need the redis as cache in air-mode
    auto gatewayFactory = std::make_shared<GatewayFactory>();
    auto gateway = gatewayFactory->buildGateway(
        ppc::protocol::NodeArch::AIR, config, nullptr, messageFactory, threadPool);
    gateway->registerFront(front->selfEndPoint(), front);
    gateway->start();

    front->setGatewayInterface(gateway);
    front->start();

    std::atomic<int> flag = 0;
    front->registerMessageHandler(uint8_t(TaskType::PSI), uint8_t(TaskAlgorithmType::CM_PSI_2PC),
        [&flag](const PPCMessageFace::Ptr& _message) {
            std::cout << "message received\n"
                      << "type: " << unsigned(_message->messageType()) << std::endl;
            std::cout << "sender: " << _message->sender() << std::endl;
            std::cout << "seq: " << _message->seq() << std::endl;
            flag++;
        });

    auto taskID = "02345678";
    auto info = std::make_shared<GatewayTaskInfo>();
    info->taskID = taskID;
    info->serviceEndpoint = "endpoint1001";
    front->notifyTaskInfo(taskID);

    auto message = buildMessage(taskID, 0);
    std::cout << "send message\n"
              << "type: " << unsigned(message->messageType()) << std::endl;
    std::cout << "sender: " << message->sender() << std::endl;
    std::cout << "seq: " << message->seq() << std::endl;
    front->asyncSendMessage("1001", message, 0, nullptr, nullptr);

    auto taskID1 = "12345678";
    auto message1 = buildMessage(taskID1, 1);
    std::cout << "send message1\n"
              << "type: " << unsigned(message1->messageType()) << std::endl;
    std::cout << "sender: " << message1->sender() << std::endl;
    std::cout << "seq: " << message1->seq() << std::endl;
    front->asyncSendMessage("1001", message1, 0, nullptr, nullptr);

    auto message2 = buildMessage(taskID1, 2);
    std::cout << "send message2\n"
              << "type: " << unsigned(message2->messageType()) << std::endl;
    std::cout << "sender: " << message2->sender() << std::endl;
    std::cout << "seq: " << message2->seq() << std::endl;
    front->asyncSendMessage("1001", message2, 0, nullptr, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    auto info1 = std::make_shared<GatewayTaskInfo>();
    info->taskID = taskID1;
    info->serviceEndpoint = "endpoint1001";
    front->notifyTaskInfo(taskID1);

    while (flag != 3)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
#endif
