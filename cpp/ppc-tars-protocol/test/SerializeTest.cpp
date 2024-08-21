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

#include "TaskInfo.h"
#include "ppc-protocol/src/PPCMessage.h"
#include "ppc-tars-protocol/ppc-tars-protocol/TarsSerialize.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppctars;
using namespace ppctars::serialize;
using namespace ppc::protocol;
using namespace ppc::front;

using namespace bcos;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(TarsProtocolTest, TestPromptFixture)


BOOST_AUTO_TEST_CASE(test_labeledPSIDataStructure)
{
    TaskInfo taskInfo;
    taskInfo.taskID = "123fdsa456";
    taskInfo.serviceEndpoint = "234f567dfaj";

    auto encodedData = std::make_shared<bytes>();
    serialize::encode(taskInfo, *encodedData);

    TaskInfo taskInfoD;
    serialize::decode(*encodedData, taskInfoD);
    BOOST_CHECK(taskInfoD == taskInfo);

    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto queryMessage = messageFactory->buildPPCMessage(
        uint8_t(1), uint8_t(2), "345", std::make_shared<bcos::bytes>());
    queryMessage->setMessageType(uint8_t(6));
    ppctars::serialize::encode(taskInfo, *queryMessage->data());
    auto buffer = std::make_shared<bytes>();
    queryMessage->encode(*buffer);

    auto newMessage = messageFactory->buildPPCMessage();
    newMessage->decode(buffer);

    TaskInfo taskInfoN;
    serialize::decode(*newMessage->data(), taskInfoN);
    BOOST_CHECK(taskInfoN == taskInfo);
}

BOOST_AUTO_TEST_SUITE_END()
