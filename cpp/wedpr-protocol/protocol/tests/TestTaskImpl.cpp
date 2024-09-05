/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file TestTaskImpl.cpp
 * @author: yujiechen
 * @date 2022-10-19
 */

#include "protocol/src/JsonTaskImpl.h"
#include "test-utils/TaskMock.h"
#include <bcos-utilities/Exceptions.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc;
using namespace ppc::protocol;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(taskImplTest, TestPromptFixture)

inline void testTask(TaskFactory::Ptr _taskFactory, std::string _selfPartyID, int _peerPartySize)
{
    auto task = _taskFactory->createTask();
    task->setId("testTask");
    task->setType((uint8_t)ppc::protocol::TaskType::PSI);
    task->setParam("taskParam");
    task->setSyncResultToPeer(true);

    // mock self-party
    auto party = mockParty((uint16_t)ppc::protocol::PartyType::Server, _selfPartyID, "selfParty",
        "dataSelf", ppc::protocol::DataResourceType::FILE, "testPath");

    std::vector<std::vector<std::string>> rawData(2);
    for (uint32_t i = 0; i < 10; i++)
    {
        rawData[0].emplace_back(std::to_string(i));
        rawData[1].emplace_back(std::to_string(i));
    }
    party->mutableDataResource()->setRawData(rawData);

    task->setSelf(party);
    // mock peers-party
    for (int i = 0; i < _peerPartySize; i++)
    {
        std::string partyID = "party" + std::to_string(i);
        party = mockParty((uint16_t)ppc::protocol::PartyType::Server, partyID, partyID, "dataParty",
            ppc::protocol::DataResourceType::FILE, "testPath");
        task->addParty(party);
    }
    // encode
    auto encodeData = task->encode();
    // decode
    auto decodedTask = _taskFactory->createTask(encodeData);
    // check the result
    checkTask(decodedTask, task);

    // invalid case
    task->setSelf(nullptr);
    encodeData = task->encode();
    // decode exception for without selfParty
    BOOST_CHECK_THROW(_taskFactory->createTask(encodeData), bcos::InvalidParameter);
}

BOOST_AUTO_TEST_CASE(testJsonTaskImpl)
{
    std::string selfPartyID = "selfParty";
    auto factory = std::make_shared<JsonTaskFactory>(selfPartyID);
    testTask(factory, selfPartyID, 10);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
