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
 * @file GatewayTest.cpp
 * @author: shawnhe
 * @date 2022-10-28
 */

#if 0
#include "ppc-gateway/ppc-gateway/Gateway.h"
#include "MockCache.h"
#include "ppc-gateway/ppc-gateway/"
#include "ppc-gateway/ppc-gateway/GatewayConfigContext.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppctars;
using namespace ppc::front;
using namespace ppc::gateway;
using namespace ppc::storage;
using namespace ppc::protocol;
using namespace ppc::mock;

using namespace bcos;
using namespace bcos::test;
using namespace ppc::tools;


BOOST_FIXTURE_TEST_SUITE(GatewayTest, TestPromptFixture)


BOOST_AUTO_TEST_CASE(test_readConfig)
{
    auto ppcConfig = std::make_shared<PPCConfig>();
    std::string configPath = "../../../ppc-gateway/test/data/config0.ini";
    ppcConfig->loadGatewayConfig(configPath);
    auto config = std::make_shared<GatewayConfigContext>(ppcConfig);
    auto const& gatewayConfig = ppcConfig->gatewayConfig();
    BOOST_CHECK(gatewayConfig.networkConfig.disableSsl == true);
    BOOST_CHECK(config->contextConfig() == nullptr);
    BOOST_CHECK(gatewayConfig.networkConfig.listenIp == "0.0.0.0");
    BOOST_CHECK(gatewayConfig.networkConfig.listenPort == 34745);
    BOOST_CHECK(gatewayConfig.networkConfig.threadPoolSize == 16);
    BOOST_CHECK(gatewayConfig.cacheStorageConfig.host == "127.0.0.1");
    BOOST_CHECK(gatewayConfig.cacheStorageConfig.port == 6379);
    BOOST_CHECK(gatewayConfig.cacheStorageConfig.password == "123456");
    BOOST_CHECK(gatewayConfig.cacheStorageConfig.pool == 16);
    BOOST_CHECK(gatewayConfig.cacheStorageConfig.database == 1);
    BOOST_CHECK(gatewayConfig.agencies.at("1001").size() == 2);
    BOOST_CHECK(gatewayConfig.agencies.at("1002").size() == 2);
}

BOOST_AUTO_TEST_CASE(test_taskManager)
{
    auto cache = std::make_shared<MockCache>();
    auto taskManager = std::make_shared<TaskManager>(std::make_shared<boost::asio::io_service>());
    taskManager->registerTaskInfo("1001", "endpoint1001");
    BOOST_CHECK(taskManager->getServiceEndpoint("1001") == "endpoint1001");

    taskManager->registerTaskInfo("1002", "endpoint1002");
    BOOST_CHECK(taskManager->getServiceEndpoint("1002") == "endpoint1002");

    BOOST_CHECK(taskManager->getServiceEndpoint("1003").empty());
}

BOOST_AUTO_TEST_CASE(test_frontNodeManager)
{
    auto threadPool = std::make_shared<ThreadPool>("TEST_POOL_MODULE", 4);
    auto frontFactory = std::make_shared<FrontFactory>("1001", threadPool);

    auto frontNodeManager = std::make_shared<FrontNodeManager>();

    auto ioService = std::make_shared<boost::asio::io_service>();
    frontNodeManager->registerFront("1001", frontFactory->buildFront(ioService));
    auto node1 = frontNodeManager->getFront("1001");
    auto node2 = frontNodeManager->getFront("1001");
    BOOST_CHECK(node1 && node2);
}

BOOST_AUTO_TEST_SUITE_END()
#endif
