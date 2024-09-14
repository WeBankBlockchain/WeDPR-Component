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
 * @file MessageTest.cpp
 * @author: shawnhe
 * @date 2022-10-28
 */

#include "protocol/src/v1/MessageHeaderImpl.h"
#include "protocol/src/v1/MessageImpl.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::protocol;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(MessageTest, TestPromptFixture)

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
    if (msg->payload())
    {
        BOOST_CHECK(*(msg->payload()) == *(decodedMsg->payload()));
    }
}

Message::Ptr fakeMsg(MessageBuilder::Ptr msgBuilder, int version, std::string const& traceID,
    std::string const& srcGwNode, std::string const& dstGwNode, int packetType, int ttl, int ext,
    std::string topic, std::string const& componentType, bcos::bytes const& srcNode,
    std::string const& srcInst, bcos::bytes const& dstNode, std::string const& dstInst,
    std::shared_ptr<bcos::bytes> payload)
{
    auto msg = msgBuilder->build();
    msg->header()->setVersion(version);
    msg->header()->setTraceID(traceID);
    msg->header()->setSrcGwNode(srcGwNode);
    msg->header()->setDstGwNode(dstGwNode);
    msg->header()->setPacketType(packetType);
    msg->header()->setTTL(ttl);
    msg->header()->setExt(ext);
    msg->header()->optionalField()->setTopic(topic);
    msg->header()->optionalField()->setComponentType(componentType);
    msg->header()->optionalField()->setSrcNode(srcNode);
    msg->header()->optionalField()->setSrcInst(srcInst);
    msg->header()->optionalField()->setDstNode(dstNode);
    msg->header()->optionalField()->setDstInst(dstInst);
    msg->setPayload(payload);
    return msg;
}

void checkEncodeDecode(MessageBuilder::Ptr msgBuilder, Message::Ptr const& msg)
{
    // encode
    bcos::bytes encodedData;
    msg->encode(encodedData);

    // decode
    auto decodedMsg = msgBuilder->build(bcos::ref(encodedData));
    checkMsg(msg, decodedMsg);
}

BOOST_AUTO_TEST_CASE(testMessage)
{
    auto msgBuilder =
        std::make_shared<MessageBuilderImpl>(std::make_shared<MessageHeaderBuilderImpl>());
    int version = 1000;
    int packetType = 2344;
    int ttl = 123;
    int ext = 1000;
    std::string traceID = "";
    std::string srcGwNode = "";
    std::string dstGwNode = "";
    std::string topic = "";
    std::string componentType = "";
    bcos::bytes srcNode;
    bcos::bytes dstNode;
    std::string srcInst;
    std::string dstInst;
    std::shared_ptr<bcos::bytes> payload = nullptr;

    auto msg = fakeMsg(msgBuilder, version, traceID, srcGwNode, dstGwNode, packetType, ttl, ext,
        topic, componentType, srcNode, srcInst, dstNode, dstInst, payload);
    checkEncodeDecode(msgBuilder, msg);
    // with payload
    std::string payloadData = "payloadf@#$@#$sdfs234";
    payload = std::make_shared<bcos::bytes>(payloadData.begin(), payloadData.end());
    msg = fakeMsg(msgBuilder, version, traceID, srcGwNode, dstGwNode, packetType, ttl, ext, topic,
        componentType, srcNode, srcInst, dstNode, dstInst, payload);
    checkEncodeDecode(msgBuilder, msg);

    // with payload over 65535
    for (uint32_t i = 0; i < 10000000; i++)
    {
        payload->emplace_back(i);
    }
    msg = fakeMsg(msgBuilder, version, traceID, srcGwNode, dstGwNode, packetType, ttl, ext, topic,
        componentType, srcNode, srcInst, dstNode, dstInst, payload);
    checkEncodeDecode(msgBuilder, msg);


    // with header router
    traceID = "1233";
    srcGwNode = "srcGwNode";
    dstGwNode = "dstGwNode";
    msg = fakeMsg(msgBuilder, version, traceID, srcGwNode, dstGwNode, packetType, ttl, ext, topic,
        componentType, srcNode, srcInst, dstNode, dstInst, payload);
    checkEncodeDecode(msgBuilder, msg);

    // with optional field
    std::string srcNodeData = "sdwerwer";
    srcNode = bcos::bytes(srcNodeData.begin(), srcNodeData.end());
    std::string dstNodeData = "dstswerwer";
    dstNode = bcos::bytes(dstNodeData.begin(), dstNodeData.end());
    dstInst = "dstInst";
    srcInst = "srcInst";
    componentType = "compp,ad";
    topic = "topcisdf";

    msg = fakeMsg(msgBuilder, version, traceID, srcGwNode, dstGwNode, packetType, ttl, ext, topic,
        componentType, srcNode, srcInst, dstNode, dstInst, payload);
    msg->setPacketType((uint16_t)ppc::gateway::GatewayPacketType::P2PMessage);
    checkEncodeDecode(msgBuilder, msg);
}

BOOST_AUTO_TEST_SUITE_END()