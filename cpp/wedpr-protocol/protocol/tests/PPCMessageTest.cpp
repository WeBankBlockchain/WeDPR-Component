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
 * @file PPCMessageTest.cpp
 * @author: shawnhe
 * @date 2022-10-28
 */

#include "ppc-protocol/src/PPCMessage.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::front;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(PPCMessageTest, TestPromptFixture)

void testEncodeAndDecode(PPCMessageFace::Ptr _message)
{
    auto payload = std::make_shared<bcos::bytes>();
    _message->encode(*payload);

    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto newMsg = messageFactory->buildPPCMessage(payload);

    BOOST_CHECK(newMsg->version() == _message->version());
    BOOST_CHECK(newMsg->taskType() == _message->taskType());
    BOOST_CHECK(newMsg->algorithmType() == _message->algorithmType());
    BOOST_CHECK(newMsg->messageType() == _message->messageType());
    BOOST_CHECK(newMsg->seq() == _message->seq());
    BOOST_CHECK(newMsg->taskID() == _message->taskID());
    BOOST_CHECK(newMsg->sender() == _message->sender());
    BOOST_CHECK(newMsg->ext() == _message->ext());
    BOOST_CHECK(newMsg->uuid() == _message->uuid());
    BOOST_CHECK(newMsg->data()->size() == _message->data()->size());
    auto newMsgHeader = newMsg->header();
    auto messageHeader = _message->header();
    BOOST_CHECK(newMsgHeader.size() == messageHeader.size());
    BOOST_CHECK(newMsgHeader["x-http-session"] == "111111");
    BOOST_CHECK(messageHeader["x-http-session"] == "111111");
    BOOST_CHECK(newMsgHeader["x-http-request"] == "2222222");
    BOOST_CHECK(messageHeader["x-http-request"] == "2222222");
}

BOOST_AUTO_TEST_CASE(test_ppcMesage)
{
    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto message = messageFactory->buildPPCMessage();
    message->setVersion(1);
    message->setTaskType(uint8_t(protocol::TaskType::PSI));
    message->setAlgorithmType(uint8_t(protocol::PSIAlgorithmType::CM_PSI_2PC));
    message->setMessageType(4);
    message->setSeq(5);
    message->setTaskID("12345678");
    message->setSender("1001");
    message->setExt(10);
    message->setUuid("uuid1245");
    message->setData(std::make_shared<bcos::bytes>(10, 'a'));
    std::map<std::string, std::string> head = {
        {"x-http-session", "111111"}, {"x-http-request", "2222222"}};
    message->setHeader(head);
    testEncodeAndDecode(message);
}

BOOST_AUTO_TEST_SUITE_END()