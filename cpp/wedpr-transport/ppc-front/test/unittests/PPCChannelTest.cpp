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
 * @file PPCChannelTest.cpp
 * @author: shawnhe
 * @date 2022-10-29
 */
#if 0

#include "ppc-front/ppc-front/PPCChannel.h"
#include "ppc-front/ppc-front/PPCChannelManager.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::front;

using namespace bcos;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(PPCChannelTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(test_ppcChannel)
{
    auto threadPool = std::make_shared<ThreadPool>("TEST_POOL_MODULE", 4);
    auto ioService = std::make_shared<boost::asio::io_service>();
    auto front = std::make_shared<Front>(ioService, "1001", threadPool);

    auto channelManager = std::make_shared<PPCChannelManager>(ioService, front);
    // Note: must start here, otherwise the ioservice will not work
    //front->start();

    // register message handler
    channelManager->registerMsgHandlerForChannel(
        uint8_t(protocol::TaskType::PSI), uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC));

    // receive message
    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto message = messageFactory->buildPPCMessage();
    message->setVersion(1);
    message->setTaskType(uint8_t(protocol::TaskType::PSI));
    message->setAlgorithmType(uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC));
    message->setMessageType(4);
    message->setSeq(5);
    message->setTaskID("T_123456");
    message->setSender("1001");
    message->setData(std::make_shared<bcos::bytes>(10, 'a'));
    front->onReceiveMessage(message, nullptr);

    auto payload = std::make_shared<bcos::bytes>();
    message->encode(*payload);
    auto newMsg = messageFactory->buildPPCMessage(payload);
    newMsg->setSeq(6);
    front->onReceiveMessage(newMsg, nullptr);

    // build channel
    auto channel = channelManager->buildChannelForTask("T_123456");

    std::atomic<int> flag = 0;

    channel->asyncReceiveMessage(
        4, 5, 1, [&flag](bcos::Error::Ptr, const PPCMessageFace::Ptr& _message) {
            BOOST_CHECK(_message->seq() == 5);
            BOOST_CHECK(_message->sender() == "1001");
            BOOST_CHECK(_message->data()->size() == 10);
            flag++;
        });

    newMsg = messageFactory->buildPPCMessage(payload);
    newMsg->setSeq(8);
    front->onReceiveMessage(newMsg, nullptr);

    channel->asyncReceiveMessage(
        4, 6, 1, [&flag](bcos::Error::Ptr, const PPCMessageFace::Ptr& _message) {
            BOOST_CHECK(_message->seq() == 6);
            BOOST_CHECK(_message->sender() == "1001");
            BOOST_CHECK(_message->data()->size() == 10);
            flag++;
        });

    // timeout
    channel->asyncReceiveMessage(
        4, 7, 1, [&flag](const bcos::Error::Ptr& _error, const PPCMessageFace::Ptr& _message) {
            BOOST_CHECK(_error->errorCode() == protocol::PPCRetCode::TIMEOUT);
            flag++;
        });

    channel->asyncReceiveMessage(
        4, 8, 1, [&flag](bcos::Error::Ptr, const PPCMessageFace::Ptr& _message) {
            BOOST_CHECK(_message->seq() == 8);
            BOOST_CHECK(_message->sender() == "1001");
            BOOST_CHECK(_message->data()->size() == 10);
            flag++;
        });

    while (flag != 4)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //front->stop();
}
BOOST_AUTO_TEST_SUITE_END()
#endif