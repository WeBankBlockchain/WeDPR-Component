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

#include "protocol/src/PPCMessage.h"
#include "ppc-framework/protocol/Protocol.h"
#include "protocol/src/v1/MessageHeaderImpl.h"
#include "protocol/src/v1/MessageImpl.h"
#include "protocol/src/v1/MessagePayloadImpl.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::front;
using namespace bcos::test;
using namespace ppc::protocol;

BOOST_FIXTURE_TEST_SUITE(PPCMessageTest, TestPromptFixture)

void checkPPCMessage(PPCMessageFace::Ptr _message, PPCMessageFace::Ptr newMsg)
{
    BOOST_CHECK(newMsg->version() == _message->version());
    BOOST_CHECK(newMsg->taskType() == _message->taskType());
    BOOST_CHECK(newMsg->algorithmType() == _message->algorithmType());
    BOOST_CHECK(newMsg->messageType() == _message->messageType());
    BOOST_CHECK(newMsg->seq() == _message->seq());
    BOOST_CHECK(newMsg->taskID() == _message->taskID());
    BOOST_CHECK(newMsg->sender() == _message->sender());
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

void checkPayloadMsg(MessagePayload::Ptr payloadMsg, MessagePayload::Ptr decodedPayload)
{
    BOOST_CHECK(payloadMsg->version() == decodedPayload->version());
    BOOST_CHECK(payloadMsg->seq() == decodedPayload->seq());
    BOOST_CHECK(payloadMsg->traceID() == decodedPayload->traceID());
    BOOST_CHECK(payloadMsg->data() == decodedPayload->data());
    BOOST_CHECK(payloadMsg->ext() == decodedPayload->ext());
}

void checkMsg(Message::Ptr msg, Message::Ptr decodedMsg)
{
    if (msg->header())
    {
        BOOST_CHECK(msg->header()->version() == decodedMsg->header()->version());
        BOOST_CHECK(msg->header()->traceID() == decodedMsg->header()->traceID());
        BOOST_CHECK(msg->header()->srcGwNode() == decodedMsg->header()->srcGwNode());
        BOOST_CHECK(msg->header()->dstGwNode() == decodedMsg->header()->dstGwNode());
        BOOST_CHECK(msg->header()->packetType() == decodedMsg->header()->packetType());
        BOOST_CHECK(msg->header()->ttl() == decodedMsg->header()->ttl());
        BOOST_CHECK(msg->header()->ext() == decodedMsg->header()->ext());
    }
    auto routeInfo = msg->header()->optionalField();
    auto decodedRouteInfo = decodedMsg->header()->optionalField();
    if (routeInfo)
    {
        BOOST_CHECK(routeInfo->topic() == decodedRouteInfo->topic());
        BOOST_CHECK(routeInfo->componentType() == decodedRouteInfo->componentType());
        BOOST_CHECK(routeInfo->srcNode() == decodedRouteInfo->srcNode());
        BOOST_CHECK(routeInfo->srcInst() == decodedRouteInfo->srcInst());
        BOOST_CHECK(routeInfo->dstNode() == decodedRouteInfo->dstNode());
        BOOST_CHECK(routeInfo->dstInst() == decodedRouteInfo->dstInst());
    }
}

PPCMessageFace::Ptr fakePPCMessage(PPCMessageFactory::Ptr messageFactory, int version, int taskType,
    int algorithmType, int messageType, std::string const& taskID, int seq, std::string const& uuid,
    std::string const& srcInst, std::shared_ptr<bcos::bytes> data,
    std::map<std::string, std::string> const& header)
{
    auto message = messageFactory->buildPPCMessage();
    message->setVersion(version);
    message->setTaskType(taskType);
    message->setAlgorithmType(algorithmType);
    message->setMessageType(messageType);
    message->setTaskID(taskID);
    message->setSeq(seq);
    message->setUuid(uuid);
    message->setSender(srcInst);
    message->setData(data);
    message->setHeader(header);
    return message;
}


BOOST_AUTO_TEST_CASE(test_ppcMesage)
{
    int version = 1;
    int taskType = uint8_t(protocol::TaskType::PSI);
    int algorithmType = uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC);
    int messageType = 4;
    std::string taskID = "12345678";
    int seq = 5;
    std::string uuid = "uuid1245";
    std::string srcInst = "from";
    std::string dstInst = "dst";
    auto data = std::make_shared<bcos::bytes>(10, 'a');
    std::map<std::string, std::string> head = {
        {"x-http-session", "111111"}, {"x-http-request", "2222222"}};

    auto messageFactory = std::make_shared<PPCMessageFactory>();
    auto ppcMessage = fakePPCMessage(messageFactory, version, taskType, algorithmType, messageType,
        taskID, seq, uuid, srcInst, data, head);

    auto payloadBuilder = std::make_shared<MessagePayloadBuilderImpl>();
    auto msgBuilder =
        std::make_shared<MessageBuilderImpl>(std::make_shared<MessageHeaderBuilderImpl>());

    auto msg = messageFactory->buildMessage(msgBuilder, payloadBuilder, ppcMessage);
    auto payloadMsg = messageFactory->buildMessage(payloadBuilder, ppcMessage);
    bcos::bytes payloadData;
    payloadMsg->encode(payloadData);

    auto decodedPayload = payloadBuilder->build(bcos::ref(payloadData));
    checkPayloadMsg(payloadMsg, decodedPayload);

    auto decodedPayload2 = payloadBuilder->build(bcos::ref(*(msg->payload())));
    checkPayloadMsg(payloadMsg, decodedPayload);

    bcos::bytes msgData;
    msg->encode(msgData);

    auto decodedMsg = msgBuilder->build(bcos::ref(msgData));
    checkMsg(msg, decodedMsg);

    decodedMsg->setFrontMessage(decodedPayload2);
    auto decodedPPCMsg = messageFactory->decodePPCMessage(decodedMsg);
    checkPPCMessage(ppcMessage, decodedPPCMsg);

    // invalid case
    std::string invalidStr = "sdfsinvalidsfwre";
    bcos::bytes invalidData(invalidStr.begin(), invalidStr.end());
    BOOST_CHECK_THROW(msgBuilder->build(bcos::ref(invalidData)), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()