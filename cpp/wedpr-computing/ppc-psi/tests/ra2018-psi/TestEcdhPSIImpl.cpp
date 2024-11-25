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
 * @file TestEcdhImpl.cpp
 * @author: yujiechen
 * @date 2022-12-29
 */
#include "mock/Common.h"
#include "mock/EcdhPSIFixture.h"
// Note: it's better not to depends on the task-impl
#include "ppc-io/src/FileLineReader.h"
#include "protocol/src/JsonTaskImpl.h"
#include "test-utils/TaskMock.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::psi;
using namespace bcos;
using namespace bcos::test;
using namespace ppc::crypto;
using namespace ppc::tools;
using namespace ppc::protocol;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(EcdhPSIImplTest, TestPromptFixture)
void testEcdhImplFunc(int64_t _dataBatchSize, std::string const& _serverPSIDataSource,
    std::string const& _clientPSIDataSource, std::string const& _resultDataPath,
    bool _expectOfflineSuccess, bool _expectPSISuccess,
    std::vector<std::string>& _expectedPSIResult, int _expectedErrorCode = 0,
    bool _mismatchedTaskID = false)
{
    auto factory = std::make_shared<FakeEcdhPSIFactory>();
    // the server config, with default setting
    auto serverConfig = std::make_shared<PPCConfig>();
    serverConfig->mutableEcdhPSIConfig().dataBatchSize = _dataBatchSize;
    serverConfig->setPrivateKey(factory->cryptoBox()->eccCrypto()->generateRandomScalar());

    // the client config, with default setting
    auto clientConfig = std::make_shared<PPCConfig>();
    clientConfig->mutableEcdhPSIConfig().dataBatchSize = _dataBatchSize;
    clientConfig->setPrivateKey(factory->cryptoBox()->eccCrypto()->generateRandomScalar());

    // fake the server
    std::string serverAgencyName = "server";
    auto serverPSI = factory->createEcdhPSI(serverAgencyName, serverConfig);

    // fake the client
    std::string clientAgencyName = "client";
    auto clientPSI = factory->createEcdhPSI(clientAgencyName, clientConfig);

    std::vector<std::string> agencyList = {serverAgencyName, clientAgencyName};
    auto serverFront = std::dynamic_pointer_cast<FakeFront>(serverPSI->psiConfig()->front());
    serverFront->setAgencyList(agencyList);

    auto clientFront = std::dynamic_pointer_cast<FakeFront>(clientPSI->psiConfig()->front());
    clientFront->setAgencyList(agencyList);

    // register the server-psi into the front
    factory->front()->registerEcdhPSI(serverAgencyName, serverPSI);
    factory->front()->registerEcdhPSI(clientAgencyName, clientPSI);
    // the server party
    std::string resourceID = "dataResource";
    std::string clientResourceID = resourceID;
    std::string serverResourceID = resourceID;
    auto serverParty = mockParty((uint16_t)PartyType::Server, serverAgencyName, "serverParty",
        serverResourceID, DataResourceType::FILE, _serverPSIDataSource);
    // the clientParty
    auto clientParty = mockParty((uint16_t)PartyType::Client, clientAgencyName, "clientParty",
        clientResourceID, DataResourceType::FILE, _clientPSIDataSource);
    auto outputDesc = std::make_shared<DataResourceDesc>();
    outputDesc->setPath(_resultDataPath);
    clientParty->mutableDataResource()->setOutputDesc(outputDesc);
    // generate client task
    auto clientPSITask = std::make_shared<JsonTaskImpl>(clientAgencyName);
    std::string taskID = "runPSI";
    clientPSITask->setId(taskID);
    clientPSITask->setEnableOutputExists(true);
    clientPSITask->setEnableOutputExists(true);
    clientPSITask->setType((int8_t)TaskType::PSI);
    clientPSITask->setAlgorithm((int8_t)TaskAlgorithmType::ECDH_PSI_2PC);
    clientPSITask->setSelf(clientParty);
    clientPSITask->addParty(serverParty);

    // generate server task
    auto serverPSITask = std::make_shared<JsonTaskImpl>(serverAgencyName);
    serverPSITask->setSelf(serverParty);
    if (_mismatchedTaskID)
    {
        serverPSITask->setId(taskID + "-mismatch");
    }
    else
    {
        serverPSITask->setId(taskID);
    }
    serverPSITask->setEnableOutputExists(true);
    serverPSITask->setEnableOutputExists(true);
    serverPSITask->setType((int8_t)TaskType::PSI);
    serverPSITask->setAlgorithm((int8_t)TaskAlgorithmType::ECDH_PSI_2PC);
    serverPSITask->addParty(clientParty);
    // run psi
    testPSI(factory, serverPSI, clientPSI, serverPSITask, clientPSITask, _expectPSISuccess,
        _expectedPSIResult, _expectedErrorCode);
}

void testNormalCase(std::string const& _outputPrefix, int64_t _dataBatchSize)
{
    std::string serverDataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/fullevaluate.txt";
    std::string clientDataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/psi.txt";
    std::string outputPath = _outputPrefix + "psiResult.txt";
    std::vector<std::string> expectedResult;
    for (int i = 1; i < 49; i++)
    {
        expectedResult.emplace_back(std::to_string(i));
    }
    testEcdhImplFunc(
        _dataBatchSize, serverDataPath, clientDataPath, outputPath, true, true, expectedResult, 0);
}

BOOST_AUTO_TEST_CASE(testNormalEcdhPSICase)
{
    std::cout << "### testNormalEcdhPSICase, batch-size: " << 1000 << std::endl;
    testNormalCase("normal-ecdh-psi-1000-batch-", 1000);
    std::cout << "### testNormalEcdhPSICase, batch-size: " << 1000 << " success!" << std::endl;

    std::cout << "### testNormalEcdhPSICase, batch-size: " << 20 << std::endl;
    testNormalCase("normal-ecdh-psi-20-batch-", 20);
    std::cout << "### testNormalEcdhPSICase, batch-size: " << 20 << " success!" << std::endl;
}

BOOST_AUTO_TEST_CASE(testECDHMissingResourceDataCase)
{
    std::string serverPSIDataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/fullevaluate-missing.txt";
    std::string clientPSIDataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/psi.txt";
    std::string outputPath = "psiResult_MissingResourceData.txt";
    std::vector<std::string> expectedResult;
    testEcdhImplFunc(1000, serverPSIDataPath, clientPSIDataPath, outputPath, true, false,
        expectedResult, (int)PSIRetCode::HandleTaskError);
}
BOOST_AUTO_TEST_CASE(testECDHMisMatchTaskID)
{
    std::string serverPSIDataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/fullevaluate.txt";
    std::string clientPSIDataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/psi.txt";
    std::string outputPath = "psiResult_MismatchTaskID.txt";
    std::vector<std::string> expectedResult;
    testEcdhImplFunc(1000, serverPSIDataPath, clientPSIDataPath, outputPath, true, false,
        expectedResult, (int)PSIRetCode::TaskNotFound, true);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test