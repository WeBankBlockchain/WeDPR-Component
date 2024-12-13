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
 * @file GatewayNodeInfoImplTest.cpp
 * @author: yujiechen
 * @date 2024-09-06
 */
#include "ppc-gateway/gateway/router/GatewayNodeInfoImpl.h"
#include "ppc-gateway/gateway/router/GatewayNodeInfo.h"
#include "protobuf/src/NodeInfoImpl.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <tbb/parallel_for.h>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::protocol;
using namespace ppc::gateway;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(NodeInfoTest, TestPromptFixture)

INodeInfo::Ptr fakeNodeInfo(INodeInfoFactory::Ptr factory, std::string const& nodeID,
    std::string const& endPoint, std::set<std::string> const& components)
{
    auto nodeInfo =
        factory->build(bcos::bytesConstRef((bcos::byte*)nodeID.data(), nodeID.size()), endPoint);
    // nodeInfo->setNodeID(bcos::bytesConstRef((bcos::byte*)nodeID.data(), nodeID.size()));
    // nodeInfo->setEndPoint(endPoint);
    nodeInfo->setComponents(components);
    return nodeInfo;
}

void checkNodeInfo(INodeInfo::Ptr nodeInfo, INodeInfo::Ptr decodedNodeInfo)
{
    BOOST_CHECK(nodeInfo->nodeID().toBytes() == decodedNodeInfo->nodeID().toBytes());
    BOOST_CHECK(nodeInfo->endPoint() == decodedNodeInfo->endPoint());
    auto const& components = nodeInfo->components();
    for (auto const& decodedComp : decodedNodeInfo->components())
    {
        BOOST_CHECK(components.count(decodedComp));
    }
}

void testNodeInfoEncodeDecode(INodeInfoFactory::Ptr factory, INodeInfo::Ptr nodeInfo)
{
    bcos::bytes encodedData;
    nodeInfo->encode(encodedData);

    auto decodedNodeInfo = factory->build(bcos::ref(encodedData));
    checkNodeInfo(nodeInfo, decodedNodeInfo);
}

void registerNode(GatewayNodeInfoImpl::Ptr gatewayNodeInfo, int nodeSize)
{
    auto nodeInfoFactory = std::make_shared<NodeInfoFactory>();
    std::string nodeID = "testn+NodeID";
    std::string endPoint = "testEndpoint";
    for (int i = 0; i < nodeSize; i++)
    {
        std::set<std::string> components;
        for (int j = 0; j < 100; j++)
        {
            components.insert("component_" + std::to_string(i) + "_" + std::to_string(j));
        }
        auto populatedNodeID = nodeID + std::to_string(i);
        auto nodeInfo = fakeNodeInfo(nodeInfoFactory, populatedNodeID, endPoint, components);
        testNodeInfoEncodeDecode(nodeInfoFactory, nodeInfo);
        bool updated = false;
        gatewayNodeInfo->tryAddNodeInfo(nodeInfo, updated);
    }
}

GatewayNodeInfo::Ptr fakeGatewayNodeInfo(GatewayNodeInfoFactory::Ptr factory, uint32_t statusSeq)
{
    auto gatewayNodeInfo = factory->build();
    gatewayNodeInfo->setStatusSeq(statusSeq);
    return gatewayNodeInfo;
}

void checkGatewayNodeInfo(GatewayNodeInfoImpl::Ptr gatewayNodeInfo, int expectedNodeSize,
    int expectedStatusSeq, std::string const& expectedAgency, std::string const& expectedP2pNode)
{
    BOOST_CHECK(gatewayNodeInfo->nodeSize() == expectedNodeSize);
    BOOST_CHECK(gatewayNodeInfo->statusSeq() == expectedStatusSeq);
    BOOST_CHECK(gatewayNodeInfo->agency() == expectedAgency);
    BOOST_CHECK(gatewayNodeInfo->p2pNodeID() == expectedP2pNode);

    // check concurrency
    tbb::parallel_for(tbb::blocked_range<size_t>(0U, 5), [&](auto const& range) {
        bcos::bytes dataTemp;
        gatewayNodeInfo->encode(dataTemp);
    });
    bcos::bytes data;
    gatewayNodeInfo->encode(data);

    auto decodedNodeInfo = std::make_shared<GatewayNodeInfoImpl>(bcos::ref(data));
    BOOST_CHECK(gatewayNodeInfo->nodeSize() == decodedNodeInfo->nodeSize());
    BOOST_CHECK(gatewayNodeInfo->statusSeq() == decodedNodeInfo->statusSeq());
    BOOST_CHECK(gatewayNodeInfo->agency() == decodedNodeInfo->agency());
    BOOST_CHECK(gatewayNodeInfo->p2pNodeID() == decodedNodeInfo->p2pNodeID());

    auto nodeList = decodedNodeInfo->nodeList();
    auto originNodeList = gatewayNodeInfo->nodeList();
    for (auto const& it : nodeList)
    {
        auto decodedNodeInfo = it.second;
        auto originNodeInfo = originNodeList.at(it.first);
        checkNodeInfo(originNodeInfo, decodedNodeInfo);
    }
}

BOOST_AUTO_TEST_CASE(testGatewayNodeInfo)
{
    std::string agency = "agency";
    std::string p2pNode = "p2p23";
    int statusSeq = 123343234234;
    auto factory = std::make_shared<GatewayNodeInfoFactoryImpl>(p2pNode, agency);
    auto gatewayNodeInfo =
        std::dynamic_pointer_cast<GatewayNodeInfoImpl>(fakeGatewayNodeInfo(factory, statusSeq));
    registerNode(gatewayNodeInfo, 10);
    checkGatewayNodeInfo(gatewayNodeInfo, 10, statusSeq, agency, p2pNode);
}

BOOST_AUTO_TEST_SUITE_END()