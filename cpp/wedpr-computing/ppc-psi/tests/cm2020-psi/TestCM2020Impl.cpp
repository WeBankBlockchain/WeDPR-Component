/**
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
 * @file TestCM2020Impl.cpp
 * @author: shawnhe
 * @date 2022-12-19
 */

#include "FakeCM2020PSIFactory.h"
#include "ppc-psi/src/cm2020-psi/CM2020PSIImpl.h"
#include "protocol/src/JsonTaskImpl.h"
#include "test-utils/FakeFront.h"
#include "test-utils/FileTool.h"
#include "test-utils/TaskMock.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc::psi;
using namespace bcos;
using namespace bcos::test;
using namespace ppc::crypto;
using namespace ppc::tools;
using namespace ppc::protocol;
using namespace ppc::io;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(CM2020ImplTest, TestPromptFixture)

void checkTaskPSIResult(DataResourceLoader::Ptr _resourceLoader,
    ppc::protocol::Task::ConstPtr _task, uint64_t expectedResultSize,
    std::vector<std::string> _expectedPSIResult)
{
    auto outputDesc = _task->selfParty()->dataResource()->outputDesc();
    auto reader = _resourceLoader->loadReader(outputDesc, DataSchema::String, false);
    // get all result
    auto result = reader->next(-1);
    // check the result
    std::cout << "### result size:" << result->size() << std::endl;
    std::cout << "### expectedResultSize: " << expectedResultSize << std::endl;
    BOOST_CHECK(result->size() == expectedResultSize);
    if (_expectedPSIResult.empty())
    {
        return;
    }
    for (uint64_t i = 0; i < result->size(); i++)
    {
        BOOST_CHECK(result->get<std::string>(i) == _expectedPSIResult.at(i));
    }
}

void testCM2020PSI(FakeCM2020PSIFactory::Ptr _factory, CM2020PSIImpl::Ptr _sender,
    CM2020PSIImpl::Ptr _receiver, ppc::protocol::Task::ConstPtr _senderPsiTask,
    ppc::protocol::Task::ConstPtr _receiverPsiTask,
    std::vector<std::string> const& _expectedPSIResult, bool _expectedSuccess,
    int _expectedErrorCode = 0)
{
    std::atomic<int> flag = 0;
    _sender->asyncRunTask(_senderPsiTask, [_senderPsiTask, _expectedSuccess, _expectedErrorCode,
                                              &flag](ppc::protocol::TaskResult::Ptr&& _response) {
        if (_expectedSuccess)
        {
            BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
            BOOST_CHECK(_response->taskID() == _senderPsiTask->id());
            auto result = _response->error();
            BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        }
        else
        {
            BOOST_CHECK(_response->error() != nullptr);
            auto result = _response->error();
            BOOST_CHECK(result != nullptr);
            BOOST_CHECK(_response->error()->errorCode() == _expectedErrorCode);
        }
        flag++;
    });

    _receiver->asyncRunTask(_receiverPsiTask,
        [_receiverPsiTask, _expectedSuccess, &flag](ppc::protocol::TaskResult::Ptr&& _response) {
            if (_expectedSuccess)
            {
                BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
                BOOST_CHECK(_response->taskID() == _receiverPsiTask->id());
                auto result = _response->error();
                BOOST_CHECK(result == nullptr || result->errorCode() == 0);
            }
            else
            {
                BOOST_CHECK(_response->error() != nullptr);
                auto result = _response->error();
                BOOST_CHECK(result != nullptr);
            }
            flag++;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    _sender->start();
    _receiver->start();

    // wait for the task finish and check
    while (flag < 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    _sender->stop();
    _receiver->stop();

    if (_expectedSuccess && !_expectedPSIResult.empty())
    {
        checkTaskPSIResult(_factory->resourceLoader(), _receiverPsiTask, _expectedPSIResult.size(),
            _expectedPSIResult);
    }
}


void testCM2020PSIImplFunc(const std::string& _taskID, const std::string& _params,
    bool _syncResults, PartyResource::Ptr _senderParty, PartyResource::Ptr _receiverParty,
    std::vector<std::string> const& _expectedPSIResult, bool _expectedSuccess,
    int _expectedErrorCode = 0)
{
    auto factory = std::make_shared<FakeCM2020PSIFactory>();

    // fake the sender
    std::string senderAgencyName = _senderParty->id();
    auto senderPSI = factory->createCM2020PSI(senderAgencyName);

    // fake the receiver
    std::string receiverAgencyName = _receiverParty->id();
    auto receiverPSI = factory->createCM2020PSI(receiverAgencyName);

    // register the server-psi into the front
    factory->front()->registerCM2020(senderAgencyName, senderPSI);
    factory->front()->registerCM2020(receiverAgencyName, receiverPSI);

    // trigger the psi task
    auto senderPSITask = std::make_shared<JsonTaskImpl>(senderAgencyName);
    senderPSITask->setId(_taskID);
    senderPSITask->setParam(_params);
    senderPSITask->setSelf(_senderParty);
    senderPSITask->addParty(_receiverParty);
    senderPSITask->setSyncResultToPeer(_syncResults);
    senderPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::CM_PSI_2PC);

    auto receiverPSITask = std::make_shared<JsonTaskImpl>(receiverAgencyName);
    receiverPSITask->setId(_taskID);
    receiverPSITask->setParam(_params);
    receiverPSITask->setSelf(_receiverParty);
    receiverPSITask->addParty(_senderParty);
    receiverPSITask->setSyncResultToPeer(_syncResults);
    receiverPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::CM_PSI_2PC);

    testCM2020PSI(factory, senderPSI, receiverPSI, senderPSITask, receiverPSITask,
        _expectedPSIResult, _expectedSuccess, _expectedErrorCode);
}

BOOST_AUTO_TEST_CASE(testNormalCM2020PSICase)
{
    std::string senderPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_inputs.csv";
    std::string receiverPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_inputs.csv";
    std::string senderOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_out.csv";
    std::string receiverOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_out.csv";

    uint32_t count = 513;
    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data");
    prepareInputs(senderPath, count, receiverPath, count, count);

    auto senderParty = mockParty((uint16_t)ppc::protocol::PartyType::Server, "sender",
        "senderPartyResource", "sender_inputs", DataResourceType::FILE, senderPath);
    auto senderOutputDesc = std::make_shared<DataResourceDesc>();
    senderOutputDesc->setPath(senderOutputPath);
    senderParty->mutableDataResource()->setOutputDesc(senderOutputDesc);

    auto receiverParty = mockParty((uint16_t)ppc::protocol::PartyType::Client, "receiver",
        "receiverPartyResource", "receiver_inputs", DataResourceType::FILE, receiverPath);
    auto receiverOutputDesc = std::make_shared<DataResourceDesc>();
    receiverOutputDesc->setPath(receiverOutputPath);
    receiverParty->mutableDataResource()->setOutputDesc(receiverOutputDesc);

    std::vector<std::string> expectedResult;
    for (uint32_t i = 0; i < count; i++)
    {
        expectedResult.emplace_back(std::to_string(100000 + i));
    }

    testCM2020PSIImplFunc(
        "0x12345678", "[0]", true, senderParty, receiverParty, expectedResult, true, 0);
    testCM2020PSIImplFunc(
        "0x12345678", "[1]", true, senderParty, receiverParty, expectedResult, true, 0);
    testCM2020PSIImplFunc(
        "0x12345678", "[0]", false, senderParty, receiverParty, expectedResult, true, 0);
    testCM2020PSIImplFunc(
        "0x12345678", "[1]", false, senderParty, receiverParty, expectedResult, true, 0);
}

BOOST_AUTO_TEST_CASE(testBigDataCM2020PSICase)
{
    std::string senderPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_inputs.csv";
    std::string receiverPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_inputs.csv";
    std::string senderOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_out.csv";
    std::string receiverOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_out.csv";

    uint32_t count = 1000000, common = 100;

    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data");
    prepareInputs(senderPath, count, receiverPath, count, common);

    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, senderPath);
    auto senderOutputDesc = std::make_shared<DataResourceDesc>();
    senderOutputDesc->setPath(senderOutputPath);
    senderParty->mutableDataResource()->setOutputDesc(senderOutputDesc);

    auto receiverParty = mockParty(uint16_t(PartyType::Client), "receiver", "receiverPartyResource",
        "receiver_inputs", DataResourceType::FILE, receiverPath);
    auto receiverOutputDesc = std::make_shared<DataResourceDesc>();
    receiverOutputDesc->setPath(receiverOutputPath);
    receiverParty->mutableDataResource()->setOutputDesc(receiverOutputDesc);

    std::vector<std::string> expectedResult;
    for (uint32_t i = 0; i < common; i++)
    {
        expectedResult.emplace_back(std::to_string(100000 + count - common + i));
    }

    testCM2020PSIImplFunc(
        "0x12345678", "[0]", false, senderParty, receiverParty, expectedResult, true, 0);
}

BOOST_AUTO_TEST_CASE(testErrorCM2020PSICase)
{
    std::string senderPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_inputs.csv";
    std::string receiverPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_inputs.csv";
    std::string senderOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_out.csv";
    std::string receiverOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_out.csv";

    uint32_t count = 1234;
    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data");
    prepareInputs(senderPath, count, receiverPath, count, count);

    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE,
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/not_existed.csv");

    auto senderOutputDesc = std::make_shared<DataResourceDesc>();
    senderOutputDesc->setPath(senderOutputPath);
    senderParty->mutableDataResource()->setOutputDesc(senderOutputDesc);

    auto receiverParty = mockParty(uint16_t(PartyType::Client), "receiver", "receiverPartyResource",
        "receiver_inputs", DataResourceType::FILE, receiverPath);
    auto receiverOutputDesc = std::make_shared<DataResourceDesc>();
    receiverOutputDesc->setPath(receiverOutputPath);
    receiverParty->mutableDataResource()->setOutputDesc(receiverOutputDesc);

    testCM2020PSIImplFunc("0x12345678", "[0]", false, senderParty, receiverParty,
        std::vector<std::string>(), false, -3000);
}

BOOST_AUTO_TEST_CASE(testRawDataCM2020PSICase)
{
    std::string senderPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_inputs.csv";
    std::string receiverPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_inputs.csv";
    std::string senderOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/sender_out.csv";
    std::string receiverOutputPath =
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data/receiver_out.csv";

    uint32_t count = 1234;
    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/cm2020-psi/data");
    prepareInputs(senderPath, count, receiverPath, count, count);

    auto reader = std::make_shared<FileLineReader>(senderPath);
    auto data = reader->next(-1);
    std::vector<std::vector<std::string>> sRawData(1);
    for (uint32_t i = 0; i < data->size(); i++)
    {
        sRawData[0].emplace_back(data->get<std::string>(i));
    }

    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, senderPath, sRawData);

    auto senderOutputDesc = std::make_shared<DataResourceDesc>();
    senderOutputDesc->setPath(senderOutputPath);
    senderParty->mutableDataResource()->setOutputDesc(senderOutputDesc);

    reader = std::make_shared<FileLineReader>(senderPath);
    data = reader->next(-1);
    std::vector<std::vector<std::string>> rRawData(1);
    for (uint32_t i = 0; i < data->size(); i++)
    {
        rRawData[0].emplace_back(data->get<std::string>(i));
    }

    auto receiverParty = mockParty(uint16_t(PartyType::Client), "receiver", "receiverPartyResource",
        "receiver_inputs", DataResourceType::FILE, receiverPath, rRawData);
    auto receiverOutputDesc = std::make_shared<DataResourceDesc>();
    receiverOutputDesc->setPath(receiverOutputPath);
    receiverParty->mutableDataResource()->setOutputDesc(receiverOutputDesc);

    testCM2020PSIImplFunc(
        "0x12345678", "[1]", true, senderParty, receiverParty, std::vector<std::string>(), true, 0);
}

BOOST_AUTO_TEST_CASE(testPrepareInptsCase)
{
    std::string senderPath = "/tmp/test0.csv";
    std::string receiverPath = "/tmp/test1.csv";

    uint32_t count = 1000 * 10000;
    prepareInputs(senderPath, count, receiverPath, count, 500000);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test