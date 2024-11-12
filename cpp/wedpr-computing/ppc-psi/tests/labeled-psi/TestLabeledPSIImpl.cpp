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
 * @date 2022-12-26
 */

#include "FakeLabeledPSIFactory.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-psi/src/labeled-psi/LabeledPSIImpl.h"
#include "ppc-psi/src/labeled-psi/protocol/LabeledPSIResult.h"
#include "protocol/src/JsonTaskImpl.h"
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
BOOST_FIXTURE_TEST_SUITE(LabeledPSIImplTest, TestPromptFixture)

void runSetup(LabeledPSIImpl::Ptr _sender)
{
    std::string senderPath =
        "../../../../wedpr-computing/ppc-psi/tests/labeled-psi/data/sender.csv";

    uint32_t count = 10000;
    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/labeled-psi/data");
    prepareItemsAndLabels(senderPath, count);

    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, senderPath);
    auto senderOutputDesc = std::make_shared<DataResourceDesc>();

    // fake the sender
    std::string senderAgencyName = senderParty->id();

    auto senderPSITask = std::make_shared<JsonTaskImpl>(senderAgencyName);
    senderPSITask->setId("0x00000000");
    senderPSITask->setParam(R"(["setup_sender_db","32"])");
    senderPSITask->setSelf(senderParty);
    senderPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::LABELED_PSI_2PC);

    std::atomic<int> flag = 0;
    _sender->asyncRunTask(senderPSITask, [&flag](ppc::protocol::TaskResult::Ptr&& _response) {
        BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
        auto result = _response->error();
        BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        flag++;
    });

    while (flag < 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void saveCache(LabeledPSIImpl::Ptr _sender)
{
    std::string cachePath =
        "../../../../wedpr-computing/ppc-psi/tests/labeled-psi/data/sender_cache.txt";
    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/labeled-psi/data");

    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, "");
    auto senderOutputDesc = std::make_shared<DataResourceDesc>();
    senderOutputDesc->setPath(cachePath);
    senderParty->mutableDataResource()->setOutputDesc(senderOutputDesc);

    // fake the sender
    std::string senderAgencyName = senderParty->id();

    auto senderPSITask = std::make_shared<JsonTaskImpl>(senderAgencyName);
    senderPSITask->setId("0x00000012");
    senderPSITask->setParam(R"(["save_sender_cache"])");
    senderPSITask->setSelf(senderParty);
    senderPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::LABELED_PSI_2PC);

    std::atomic<int> flag = 0;
    _sender->asyncRunTask(senderPSITask, [&flag](ppc::protocol::TaskResult::Ptr&& _response) {
        BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
        auto result = _response->error();
        BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        flag++;
    });

    while (flag < 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void loadCache(LabeledPSIImpl::Ptr _sender)
{
    std::string cachePath =
        "../../../../wedpr-computing/ppc-psi/tests/labeled-psi/data/sender_cache.txt";

    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, cachePath);

    // fake the sender
    std::string senderAgencyName = senderParty->id();

    auto senderPSITask = std::make_shared<JsonTaskImpl>(senderAgencyName);
    senderPSITask->setId("0x00000034");
    senderPSITask->setParam(R"(["load_sender_cache"])");
    senderPSITask->setSelf(senderParty);
    senderPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::LABELED_PSI_2PC);

    std::atomic<int> flag = 0;
    _sender->asyncRunTask(senderPSITask, [&flag](ppc::protocol::TaskResult::Ptr&& _response) {
        BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
        auto result = _response->error();
        BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        flag++;
    });

    while (flag < 1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void testLabeledPSI(LabeledPSIImpl::Ptr _sender, LabeledPSIImpl::Ptr _receiver,
    ppc::protocol::Task::ConstPtr _senderPsiTask, ppc::protocol::Task::ConstPtr _receiverPsiTask,
    bool _expectedSuccess, int _expectedErrorCode = 0, int _hit = 0)
{
    std::atomic<int> flag = 0;

    _receiver->asyncRunTask(_receiverPsiTask,
        [_receiverPsiTask, _expectedSuccess, _hit, &flag](protocol::TaskResult::Ptr&& _response) {
            if (_expectedSuccess)
            {
                BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
                BOOST_CHECK(_response->taskID() == _receiverPsiTask->id());
                auto result = _response->error();
                BOOST_CHECK(result == nullptr || result->errorCode() == 0);

                LabeledPSIResult::Ptr labeledPSIResult =
                    std::dynamic_pointer_cast<LabeledPSIResult>(_response);
                BOOST_CHECK(labeledPSIResult->getOutputs()[0].size() == uint(_hit));
                // std::cout << labeledPSIResult->getOutputs() << std::endl;
            }
            else
            {
                BOOST_CHECK(_response->error() != nullptr);
                auto result = _response->error();
                BOOST_CHECK(result != nullptr);
            }
            flag++;
        });

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

    // wait for the task finish and check
    while (flag < 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void testLabeledPSIImplFunc(const std::string& _taskID, const std::string& _params,
    PartyResource::Ptr _senderParty, PartyResource::Ptr _receiverParty, bool _ready, bool _testLoad,
    bool _expectedSuccess, int _expectedErrorCode = 0, int _hit = 0)
{
    auto factory = std::make_shared<FakeLabeledPSIFactory>();

    // fake the sender
    std::string senderAgencyName = _senderParty->id();
    auto senderPSI = factory->createLabeledPSI(senderAgencyName);
    senderPSI->start();
    if (_ready)
    {
        if (_testLoad)
        {
            loadCache(senderPSI);
        }
        else
        {
            runSetup(senderPSI);
            saveCache(senderPSI);
        }
    }

    // fake the receiver
    std::string receiverAgencyName = _receiverParty->id();
    auto receiverPSI = factory->createLabeledPSI(receiverAgencyName);
    receiverPSI->start();

    // register the server-psi into the front
    factory->front()->registerLabeledPSI(senderAgencyName, senderPSI);
    factory->front()->registerLabeledPSI(receiverAgencyName, receiverPSI);

    // trigger the psi task
    auto senderPSITask = std::make_shared<JsonTaskImpl>(senderAgencyName);
    senderPSITask->setId(_taskID);
    senderPSITask->setParam(_params);
    senderPSITask->setSelf(_senderParty);
    senderPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::LABELED_PSI_2PC);
    senderPSITask->addParty(_receiverParty);

    auto receiverPSITask = std::make_shared<JsonTaskImpl>(receiverAgencyName);
    receiverPSITask->setId(_taskID);
    receiverPSITask->setSelf(_receiverParty);
    receiverPSITask->setAlgorithm((uint8_t)TaskAlgorithmType::LABELED_PSI_2PC);
    receiverPSITask->addParty(_senderParty);

    testLabeledPSI(senderPSI, receiverPSI, senderPSITask, receiverPSITask, _expectedSuccess,
        _expectedErrorCode, _hit);

    senderPSI->stop();
    receiverPSI->stop();
}


BOOST_AUTO_TEST_CASE(testSenderSetup)
{
    auto factory = std::make_shared<FakeLabeledPSIFactory>();

    // fake the sender
    std::string senderAgencyName = "sender";
    auto senderPSI = factory->createLabeledPSI(senderAgencyName);

    senderPSI->start();

    runSetup(senderPSI);

    senderPSI->stop();
}


BOOST_AUTO_TEST_CASE(testLabeledPSICase)
{
    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, "");

    std::vector<std::vector<std::string>> rRawData(1);
    for (int i = 0; i < MAX_QUERY_SIZE / 4; i++)
    {
        rRawData[0].emplace_back(std::to_string(100000 + i));
        rRawData[0].emplace_back(std::to_string(100000000 + i));
    }

    auto receiverParty = mockParty(uint16_t(PartyType::Client), "", "receiverPartyResource",
        "receiver_inputs", DataResourceType::FILE, "", rRawData);

    testLabeledPSIImplFunc("0x12345678", "[\"run_labeled_psi\"]", senderParty, receiverParty, true,
        false, true, 0, MAX_QUERY_SIZE / 4);
}

BOOST_AUTO_TEST_CASE(testLabeledPSINotReady)
{
    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, "");

    std::vector<std::vector<std::string>> rRawData(1);
    rRawData[0].emplace_back("100018");

    auto receiverParty = mockParty(uint16_t(PartyType::Client), "receiver", "receiverPartyResource",
        "receiver_inputs", DataResourceType::FILE, "", rRawData);

    testLabeledPSIImplFunc("0x12345678", "[\"run_labeled_psi\"]", senderParty, receiverParty, false,
        false, false, -2001);
}

BOOST_AUTO_TEST_CASE(testLabeledPSILoacdCache)
{
    auto senderParty = mockParty(uint16_t(PartyType::Server), "sender", "senderPartyResource",
        "sender_inputs", DataResourceType::FILE, "");

    std::vector<std::vector<std::string>> rRawData(1);
    for (int i = 0; i < MAX_QUERY_SIZE / 4; i++)
    {
        rRawData[0].emplace_back(std::to_string(100000 + i));
        rRawData[0].emplace_back(std::to_string(100000000 + i));
    }

    auto receiverParty = mockParty(uint16_t(PartyType::Client), "", "receiverPartyResource",
        "receiver_inputs", DataResourceType::FILE, "", rRawData);

    testLabeledPSIImplFunc("0x12345678", "[\"run_labeled_psi\"]", senderParty, receiverParty, true,
        true, true, 0, MAX_QUERY_SIZE / 4);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test