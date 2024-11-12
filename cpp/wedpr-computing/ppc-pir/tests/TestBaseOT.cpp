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
 * @file TestCEMService.cpp
 * @author: asherli
 * @date 2023-03-13
 */
#include "FakeOtPIRFactory.h"
#include "ppc-crypto-core/src/hash/HashFactoryImpl.h"
#include "ppc-crypto/src/ecc/EccCryptoFactoryImpl.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-pir/src/BaseOT.h"
#include "ppc-pir/src/Common.h"
#include "ppc-pir/src/OtPIRImpl.h"
#include "test-utils/TaskMock.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <ostream>

using namespace ppc::pir;
using namespace ppc::crypto;
using namespace ppc::pir;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(OtPIRest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(testBaseOT)
{
    // 统计函数执行时间
    std::cout << "testBaseOT" << std::endl;

    auto eccFactory = std::make_shared<EccCryptoFactoryImpl>();
    auto hashFactory = std::make_shared<HashFactoryImpl>();
    auto hash = hashFactory->createHashImpl((int8_t)ppc::protocol::HashImplName::SHA512);
    EccCrypto::Ptr ecc = eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::P256, hash);
    // auto aysService = std::make_shared<OtPIRImpl>();
    // auto aes = std::make_shared<OpenSSLAES>(
    //         OpenSSLAES::AESType::AES128,
    //         SymCrypto::OperationMode::CBC, _seed, bcos::bytes());

    std::string datasetPath = "../../../../wedpr-computing/ppc-pir/tests/data/AysPreDataset.csv";
    // std::cout<< "aysService->prepareDataset" << std::endl;
    std::string prefix = "testmsg1";
    bcos::bytes sendObfuscatedHash(prefix.begin(), prefix.end());

    uint32_t obfuscatedOrder = 10;
    auto baseOT = std::make_shared<BaseOT>(ecc, hash);
    auto messageKeypair = baseOT->prepareDataset(sendObfuscatedHash, datasetPath);
    // for(auto iter = messageKeypair.begin(); iter != messageKeypair.end();
    // ++iter)
    // {
    //     std::string pairKey(iter->first.begin(), iter->first.end());
    //     std::string pairValue(iter->second.begin(), iter->second.end());
    //     // for(uint32_t i = 0; i < messageKeypair.size(); ++i) {
    //     // std::cout<< "pairKey:"<< pairKey << std::endl;
    //     // std::cout<< "pairValue:"<< pairValue << std::endl;
    // }
    auto start = std::chrono::high_resolution_clock::now();

    std::string choice = "testmsg1100";
    // std::cout<< "baseOT->senderGenerateCipher" << std::endl;
    auto senderMessage =
        baseOT->senderGenerateCipher(bcos::bytes(choice.begin(), choice.end()), obfuscatedOrder);
    // std::cout<< "baseOT->receiverGenerateMessage" << std::endl;
    auto receiverMessage = baseOT->receiverGenerateMessage(
        senderMessage.pointX, senderMessage.pointY, messageKeypair, senderMessage.pointZ);

    // std::cout<< "baseOT->finishSender" << std::endl;
    auto result = baseOT->finishSender(senderMessage.scalarBlidingB, receiverMessage.pointWList,
        receiverMessage.encryptMessagePair, receiverMessage.encryptCipher);

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "执行时间 time: " << duration.count() << " microseconds" << std::endl;

    if (result.size() == 0)
    {
        std::cout << "final result: message not found" << std::endl;
    }
    else
    {
        std::cout << "final result: " << std::string(result.begin(), result.end()) << std::endl;
    }
    // for(uint32_t i = 0; i < result.size(); ++i)
    // {
    //     std::cout<< std::string(result[i].begin(), result[i].end()) <<
    //     std::endl;
    // }

    BOOST_CHECK(true);
}

void testOTPIR(FakeOtPIRFactory::Ptr _factory, OtPIRImpl::Ptr _sender, OtPIRImpl::Ptr _receiver,
    ppc::protocol::Task::ConstPtr _senderPirTask, ppc::protocol::Task::ConstPtr _receiverPirTask,
    std::vector<std::string> const& _expectedPIRResult, bool _expectedSuccess,
    int _expectedErrorCode = 0)
{
    std::atomic<int> flag = 0;

    _sender->asyncRunTask(_senderPirTask, [_senderPirTask, _expectedSuccess, _expectedErrorCode,
                                              &flag](ppc::protocol::TaskResult::Ptr&& _response) {
        if (_expectedSuccess)
        {
            BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
            BOOST_CHECK(_response->taskID() == _senderPirTask->id());
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
    _sender->start();

    _receiver->asyncRunTask(_receiverPirTask,
        [_receiverPirTask, _expectedSuccess, &flag](ppc::protocol::TaskResult::Ptr&& _response) {
            if (_expectedSuccess)
            {
                BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
                BOOST_CHECK(_response->taskID() == _receiverPirTask->id());
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
    _receiver->start();

    // wait for the task finish and check
    while (flag < 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    _sender->stop();
    _receiver->stop();

    // if (_expectedSuccess && !_expectedPIRResult.empty())
    // {
    //     checkTaskPIRResult(_factory->resourceLoader(), _receiverPirTask,
    //     _expectedPIRResult.size(),
    //         _expectedPIRResult);
    // }
}

void testOTPIRImplFunc(const std::string& _taskID, const std::string& _params, bool _syncResults,
    PartyResource::Ptr _senderParty, PartyResource::Ptr _receiverParty,
    std::vector<std::string> const& _expectedPIRResult, bool _expectedSuccess,
    int _expectedErrorCode = 0)
{
    auto factory = std::make_shared<FakeOtPIRFactory>();

    // fake the sender
    std::string senderAgencyName = "sender";
    auto senderPIR = factory->createOtPIR("sender");

    // fake the receiver
    std::string receiverAgencyName = _receiverParty->id();
    auto receiverPIR = factory->createOtPIR(receiverAgencyName);

    // register the server-pir into the front
    factory->front()->registerOTPIR(senderAgencyName, senderPIR);
    factory->front()->registerOTPIR(receiverAgencyName, receiverPIR);

    // trigger the pir task
    auto senderPIRTask = std::make_shared<JsonTaskImpl>(senderAgencyName);
    senderPIRTask->setId(_taskID);
    senderPIRTask->setParam(_params);
    senderPIRTask->setSelf(_senderParty);
    senderPIRTask->addParty(_receiverParty);
    senderPIRTask->setSyncResultToPeer(_syncResults);
    senderPIRTask->setAlgorithm((uint8_t)TaskAlgorithmType::OT_PIR_2PC);
    senderPIRTask->setType((uint8_t)ppc::protocol::TaskType::PIR);

    auto receiverPIRTask = std::make_shared<JsonTaskImpl>(receiverAgencyName);
    receiverPIRTask->setId(_taskID);
    receiverPIRTask->setParam(_params);
    receiverPIRTask->setSelf(_receiverParty);
    receiverPIRTask->addParty(_senderParty);
    receiverPIRTask->setSyncResultToPeer(_syncResults);
    receiverPIRTask->setAlgorithm((uint8_t)TaskAlgorithmType::OT_PIR_2PC);
    receiverPIRTask->setType((uint8_t)ppc::protocol::TaskType::PIR);

    testOTPIR(factory, senderPIR, receiverPIR, senderPIRTask, receiverPIRTask, _expectedPIRResult,
        _expectedSuccess, _expectedErrorCode);
}

BOOST_AUTO_TEST_CASE(testNormalOtPIRCase)
{
    std::string otSearchPath = "../../../../wedpr-computing/ppc-pir/tests/data/AysPreDataset.csv";
    std::string outputPath = "../../../../wedpr-computing/ppc-pir/tests/data/output.csv";

    uint32_t count = 513;
    // prepareInputs(senderPath, count, receiverPath, count, count);

    auto senderParty = mockParty((uint16_t)ppc::protocol::PartyType::Client, "sender",
        "senderPartyResource", "sender_inputs", protocol::DataResourceType::FILE, "");
    auto senderOutputDesc = std::make_shared<DataResourceDesc>();
    senderOutputDesc->setPath(outputPath);
    senderParty->mutableDataResource()->setOutputDesc(senderOutputDesc);

    auto receiverParty = mockParty((uint16_t)ppc::protocol::PartyType::Server, "receiver",
        "receiverPartyResource", "receiver_inputs", DataResourceType::FILE, otSearchPath);

    // auto receiverOutputDesc = std::make_shared<DataResourceDesc>();
    // receiverOutputDesc->setPath(outputPath);
    // receiverParty->mutableDataResource()->setOutputDesc(receiverOutputDesc);

    std::vector<std::string> expectedResult;
    for (uint32_t i = 0; i < count; i++)
    {
        expectedResult.emplace_back(std::to_string(100000 + i));
    }

    std::string jobParams =
        "{\"searchId\":\"testmsg1100\",\"requestAgencyId\":"
        "\"receiver\",\"prefixLength\":6}";

    testOTPIRImplFunc(
        "0x12345678", jobParams, true, senderParty, receiverParty, expectedResult, true, 0);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
