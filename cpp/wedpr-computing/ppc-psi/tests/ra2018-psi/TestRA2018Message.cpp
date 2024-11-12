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
 * @file TestRA2018Message.cpp
 * @author: yujiechen
 * @date 2022-11-16
 */
#include "mock/RA2018MessageFixture.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-psi/src/ra2018-psi/Common.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::psi;
using namespace bcos;
using namespace bcos::test;
using namespace ppc::crypto;
using namespace ppc::tools;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(RA2018MessageTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(testRA2018MessageImpl)
{
    auto msgFactory = std::make_shared<RA2018MessageFactory>();
    std::string partyID = "selfParty";
    std::string resourceID = "testResource";
    int32_t version = 1000123;
    // fake the data
    std::vector<bcos::bytes> data;
    for (int i = 0; i < 100; i++)
    {
        std::string item = std::to_string(i) + "werwlerklk";
        data.emplace_back(bcos::bytes(item.begin(), item.end()));
    }

    // the evaluate message
    std::cout << "#### test EvaluateRequest" << std::endl;
    auto msg = msgFactory->createPSIMessage((uint32_t)RA2018PacketType::EvaluateRequest);
    fakeAndCheckRA2018Message(msg, msgFactory, (uint32_t)RA2018PacketType::EvaluateRequest, partyID,
        resourceID, version, data);
    std::cout << "#### test EvaluateRequest finish" << std::endl;
    // the EvaluateResponse message
    std::cout << "#### test EvaluateResponse" << std::endl;
    msg = msgFactory->createPSIMessage((uint32_t)RA2018PacketType::EvaluateResponse);
    fakeAndCheckRA2018Message(msg, msgFactory, (uint32_t)RA2018PacketType::EvaluateResponse,
        partyID, resourceID, version, data);
    std::cout << "#### test EvaluateResponse finish" << std::endl;

    // fake the cuckooFilterInfo
    std::vector<CuckooFilterInfo::Ptr> cuckooFilters;
    auto hashImpl = std::make_shared<Sha256Hash>();
    auto option = std::make_shared<CuckoofilterOption>();
    option->tagBits = 32;
    option->maxKickOutCount = 10;
    option->capacity = 20000;

    for (int i = 0; i < 10; i++)
    {
        auto filterInfo = fakeCuckooFilterInfo(i, hashImpl, option, 10000 + i);
        cuckooFilters.emplace_back(filterInfo);
    }
    // check the CuckooFilterRequest
    std::cout << "#### test CuckooFilterRequest" << std::endl;
    msg = msgFactory->createRA2018FilterMessage((uint32_t)RA2018PacketType::CuckooFilterRequest);
    fakeAndCheckRA2018FilterMessage(msg, msgFactory,
        (uint32_t)RA2018PacketType::CuckooFilterRequest, partyID, resourceID, version, data,
        cuckooFilters);
    std::cout << "#### test CuckooFilterRequest finish" << std::endl;

    // check the CuckooFilterResponse
    std::cout << "#### test CuckooFilterResponse" << std::endl;
    msg = msgFactory->createRA2018FilterMessage((uint32_t)RA2018PacketType::CuckooFilterResponse);
    fakeAndCheckRA2018FilterMessage(msg, msgFactory,
        (uint32_t)RA2018PacketType::CuckooFilterResponse, partyID, resourceID, version, data,
        cuckooFilters);
    std::cout << "#### test CuckooFilterResponse finish" << std::endl;

    // check the CancelTaskNotification
    std::cout << "#### test CancelTaskNotification" << std::endl;
    msg =
        msgFactory->createTaskNotificationMessage((uint32_t)PSIPacketType::CancelTaskNotification);
    fakeAndCheckNotificationMessage(msg, msgFactory,
        (uint32_t)PSIPacketType::CancelTaskNotification, partyID, resourceID, version, data, -10999,
        "-1000999");
    std::cout << "#### test CancelTaskNotification finish" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test