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
 * @file NodeInfoImplTest.cpp
 * @author: yujiechen
 * @date 2024-09-06
 */

#include "protobuf/src/NodeInfoImpl.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::protocol;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(NodeInfoTest, TestPromptFixture)

INodeInfo::Ptr fakeNodeInfo(INodeInfoFactory::Ptr factory, std::string const& nodeID,
    std::string const& endPoint, std::set<std::string> const& components)
{
    auto nodeInfo = factory->build();
    nodeInfo->setNodeID(bcos::bytesConstRef((bcos::byte*)nodeID.data(), nodeID.size()));
    nodeInfo->setEndPoint(endPoint);
    nodeInfo->setComponents(components);
    return nodeInfo;
}


void testNodeInfoEncodeDecode(INodeInfoFactory::Ptr factory, INodeInfo::Ptr nodeInfo)
{
    bcos::bytes encodedData;
    nodeInfo->encode(encodedData);

    auto decodedNodeInfo = factory->build(bcos::ref(encodedData));
    BOOST_CHECK(nodeInfo->nodeID().toBytes() == decodedNodeInfo->nodeID().toBytes());
    BOOST_CHECK(nodeInfo->endPoint() == decodedNodeInfo->endPoint());
    auto const& components = nodeInfo->components();
    for (auto const& decodedComp : decodedNodeInfo->components())
    {
        BOOST_CHECK(components.count(decodedComp));
    }
}

BOOST_AUTO_TEST_CASE(testNodeInfo)
{
    auto nodeInfoFactory = std::make_shared<NodeInfoFactory>();
    std::string nodeID = "testn+NodeID";
    std::string endPoint = "testEndpoint";
    std::set<std::string> components;
    for (int i = 0; i < 100; i++)
    {
        components.insert("component_" + std::to_string(i));
    }
    auto nodeInfo = fakeNodeInfo(nodeInfoFactory, nodeID, endPoint, components);
    testNodeInfoEncodeDecode(nodeInfoFactory, nodeInfo);
}

BOOST_AUTO_TEST_SUITE_END()