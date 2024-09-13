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
 * @file transport_bench.cpp
 * @desc: bench for transport
 * @author: yujiechen
 * @date 2024-09-13
 */
#include "ppc-framework/protocol/RouteType.h"
#include "wedpr-transport/sdk/src/TransportBuilder.h"
#include <thread>

using namespace ppc::front;
using namespace ppc::sdk;
using namespace ppc::protocol;

void Usage(std::string const& _appName)
{
    std::cout << _appName << " [hostIp] [listenPort] [gatewayTargets] [nodeID] [dstNode]"
              << std::endl;
    std::cout << "example:" << std::endl;
    std::cout << _appName
              << " 127.0.0.1 9020 ipv4:127.0.0.1:40600,127.0.0.1:40601 agency0Node agency1Node"
              << std::endl;
    std::cout << _appName
              << " 127.0.0.1 9021 ipv4:127.0.0.1:40620,127.0.0.1:40621 agency1Node agency0Node"
              << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        Usage(argv[0]);
        return -1;
    }
    // the hostIp
    std::string hostIp = argv[1];
    // the clientPort
    int port = atoi(argv[2]);
    std::string listenIp = "0.0.0.0";
    // the gatewayTargets
    std::string gatewayTargets = argv[3];
    // the nodeID
    std::string nodeID = argv[4];
    // the dstNode
    std::string dstNode = argv[5];

    auto transportBuilder = std::make_shared<TransportBuilder>();
    auto frontConfig = transportBuilder->buildConfig(2, nodeID);
    frontConfig->setGatewayGrpcTarget(gatewayTargets);
    EndPoint endPoint(hostIp, port);
    frontConfig->setSelfEndPoint(endPoint);
    auto transport = transportBuilder->buildProTransport(frontConfig);

    // start the transport
    std::cout << "#### start the front, detail: " << printFrontDesc(frontConfig);
    transport->start();
    auto routeInfo = transport->routeInfoBuilder()->build();
    std::string topic = "sync_testDemo";
    routeInfo->setDstNode(bcos::bytes(dstNode.begin(), dstNode.end()));
    routeInfo->setTopic(topic);
    transport->getFront()->registerTopicHandler(topic, [](Message::Ptr msg) {
        std::cout << "=== async receive message: " << printMessage(msg) << "====" << std::endl;
    });
    std::string syncTopic = "sync__" + topic;
    auto syncRouteInfo = transport->routeInfoBuilder()->build();
    syncRouteInfo->setDstNode(bcos::bytes(dstNode.begin(), dstNode.end()));
    syncRouteInfo->setTopic(syncTopic);
    // sendMessage test
    long i = 0;
    while (true)
    {
        try
        {
            std::string payload = "payload+++" + std::to_string(i);
            bcos::bytes payloadBytes = bcos::bytes(payload.begin(), payload.end());
            // async test
            transport->getFront()->asyncSendMessage((uint16_t)RouteType::ROUTE_THROUGH_NODEID,
                routeInfo, std::move(bcos::bytes(payload.begin(), payload.end())), 0, 10000,
                [](bcos::Error::Ptr error) {
                    if (error && error->errorCode() != 0)
                    {
                        std::cout << "!**** send message failed for: " << error->errorMessage()
                                  << "***!" << std::endl;
                    }
                },
                nullptr);

            // push
            auto error = transport->getFront()->push((uint16_t)RouteType::ROUTE_THROUGH_NODEID,
                syncRouteInfo, std::move(payloadBytes), 0, 10000);
            if (!error && error->errorCode() != 0)
            {
                std::cout << "!**** send message failed for: " << error->errorMessage() << "***!"
                          << std::endl;
            }
            // pop
            auto msg = transport->getFront()->pop(syncTopic, 10000);
            if (msg == nullptr)
            {
                std::cout << "try to receive message timeout" << std::endl;
            }
            else
            {
                std::cout << "=== sync receive message: " << printMessage(msg)
                          << "====" << std::endl;
            }
            i++;
        }
        catch (std::exception const& e)
        {
            std::cout << "!**** exception: " << boost::diagnostic_information(e) << "****!"
                      << std::endl;
        }
        // wait for 2s
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    };
    return 0;
}
