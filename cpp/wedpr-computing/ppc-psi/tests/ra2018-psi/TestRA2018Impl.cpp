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
 * @file TestRA2018Impl.cpp
 * @author: yujiechen
 * @date 2022-11-16
 */
#include "mock/Common.h"
#include "mock/RA2018PSIFixture.h"
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
BOOST_FIXTURE_TEST_SUITE(RA2018ImplTest, TestPromptFixture)
// offline evaluate
void testRA2018FullEvaluate(FakeRA2018PSIFactory::Ptr _factory, RA2018PSIImpl::Ptr _server,
    ppc::protocol::Task::ConstPtr _evaluateTask, bool _expectedSuccess)
{
    // test the server offline-evaluate
    std::promise<bool> completedFuture;
    _server->asyncRunTask(_evaluateTask, [_evaluateTask, _expectedSuccess, &completedFuture](
                                             ppc::protocol::TaskResult::Ptr&& _response) {
        if (_expectedSuccess)
        {
            BOOST_CHECK(_response->error() == nullptr || _response->error()->errorCode() == 0);
            BOOST_CHECK(_response->taskID() == _evaluateTask->id());
            auto result = _response->error();
            BOOST_CHECK(result == nullptr || result->errorCode() == 0);
        }
        else
        {
            BOOST_CHECK(_response->error() != nullptr);
            auto result = _response->error();
            BOOST_CHECK(result != nullptr);
        }
        completedFuture.set_value(true);
    });
    completedFuture.get_future().get();
}

void testRA2018PSIImplFunc(int _dataBatchSize, CuckoofilterOption::Ptr option,
    std::string const& _offlineDataPath, std::string const& _psiDataSource,
    std::string const& _resultDataPath, bool _expectOfflineSuccess, bool _expectPSISuccess,
    std::vector<std::string>& _expectedPSIResult, int _expectedErrorCode = 0,
    bool _misMatchDataSource = false, std::string const& _offlineInsertDataPath = "",
    std::vector<std::string> const& _appendIntersectionData = std::vector<std::string>())
{
    auto factory = std::make_shared<FakeRA2018PSIFactory>();
    // the server config, with default cache setting
    auto serverConfig = std::make_shared<PPCConfig>();
    auto& mutableServerConfig = serverConfig->mutableRA2018PSIConfig();
    mutableServerConfig.dataBatchSize = _dataBatchSize;

    serverConfig->setPrivateKey(factory->cryptoBox()->eccCrypto()->generateRandomScalar());
    mutableServerConfig.cuckooFilterOption = option;

    // the client config, with default cache setting
    auto clientConfig = std::make_shared<PPCConfig>();
    auto& mutableClientConfig = clientConfig->mutableRA2018PSIConfig();
    mutableClientConfig.cuckooFilterOption = option;
    mutableClientConfig.dataBatchSize = _dataBatchSize;

    // fake the server
    std::string serverAgencyName = "server";
    auto serverPSI = factory->createRA2018PSI(serverAgencyName, serverConfig);

    // fake the client
    std::string clientAgencyName = "client";
    auto clientPSI = factory->createRA2018PSI(clientAgencyName, clientConfig);
    // register the server-psi into the front
    factory->front()->registerRA2018(serverAgencyName, serverPSI);
    factory->front()->registerRA2018(clientAgencyName, clientPSI);

    // generate offline-evaluate task
    std::string resourceID = "dataResource";
    std::string clientResourceID = resourceID;
    std::string serverResourceID = resourceID;
    auto serverParty = mockParty((uint16_t)PartyType::Server, serverAgencyName, "serverParty",
        serverResourceID, DataResourceType::FILE, _offlineDataPath);

    auto offlineFullEvaluateTask = std::make_shared<JsonTaskImpl>(serverAgencyName);
    offlineFullEvaluateTask->setId("offlineFullEvaluate");
    offlineFullEvaluateTask->setSelf(serverParty);
    offlineFullEvaluateTask->setEnableOutputExists(true);
    // insert operation
    std::string param = "[\"data_preprocessing\", 0]";
    offlineFullEvaluateTask->setParam(param);
    // the server trigger offline-evaluate
    testRA2018FullEvaluate(factory, serverPSI, offlineFullEvaluateTask, _expectOfflineSuccess);

    /// trigger the psi task
    // the client task
    if (_misMatchDataSource)
    {
        serverResourceID = "miss-match-" + resourceID;
    }
    // update the server-resource-id
    serverParty->mutableDataResource()->setResourceID(serverResourceID);
    auto clientParty = mockParty((uint16_t)PartyType::Client, clientAgencyName, "clientParty",
        clientResourceID, DataResourceType::FILE, _psiDataSource);
    auto outputDesc = std::make_shared<DataResourceDesc>();
    outputDesc->setPath(_resultDataPath);
    clientParty->mutableDataResource()->setOutputDesc(outputDesc);
    auto clientPSITask = std::make_shared<JsonTaskImpl>(clientAgencyName);
    std::string taskID = "runPSI";
    clientPSITask->setId(taskID);
    clientPSITask->setEnableOutputExists(true);
    clientPSITask->setSelf(clientParty);
    clientPSITask->addParty(serverParty);
    param = "[\"ra2018_psi\"]";
    clientPSITask->setParam(param);

    // the server task
    auto serverPSITask = std::make_shared<JsonTaskImpl>(serverAgencyName);
    serverPSITask->setSelf(serverParty);
    serverPSITask->setEnableOutputExists(true);
    serverPSITask->setId(taskID);
    serverPSITask->setParam(param);
    serverPSITask->addParty(clientParty);
    testPSI(factory, serverPSI, clientPSI, serverPSITask, clientPSITask, _expectPSISuccess,
        _expectedPSIResult, _expectedErrorCode);

    // test insert new data into the cuckoo-filter
    if (_offlineInsertDataPath.empty() || _appendIntersectionData.empty())
    {
        return;
    }
    std::cout << "### test offline-fullevaluate-insert" << std::endl;
    offlineFullEvaluateTask->selfParty()->mutableDataResource()->mutableDesc()->setPath(
        _offlineInsertDataPath);
    offlineFullEvaluateTask->setId("insert_" + _offlineDataPath);
    // the server trigger offline-evaluate
    testRA2018FullEvaluate(factory, serverPSI, offlineFullEvaluateTask, _expectOfflineSuccess);
    std::cout << "### test offline-fullevaluate-insert finished" << std::endl;

    // trigger psi task again
    std::cout << "### test psi-insert" << std::endl;
    taskID = "insert_runPSI";
    serverPSITask->setId(taskID);
    clientPSITask->setId(taskID);
    auto expectedResult = _expectedPSIResult;
    expectedResult.insert(
        expectedResult.end(), _appendIntersectionData.begin(), _appendIntersectionData.end());
    testPSI(factory, serverPSI, clientPSI, serverPSITask, clientPSITask, _expectPSISuccess,
        expectedResult, _expectedErrorCode);
    std::cout << "### test psi-insert success" << std::endl;

    // delete the evaluated data
    std::cout << "### test offline-fullevaluate-delete" << std::endl;
    param = "[\"data_preprocessing\", 1]";
    offlineFullEvaluateTask->setId("delete-" + _offlineDataPath);
    offlineFullEvaluateTask->setParam(param);
    // the server trigger offline-evaluate
    testRA2018FullEvaluate(factory, serverPSI, offlineFullEvaluateTask, _expectOfflineSuccess);
    std::cout << "### test offline-fullevaluate-delete finished" << std::endl;

    // trigger psi after delete
    std::cout << "### test psi-delete" << std::endl;
    taskID = "delete_runPSI";
    serverPSITask->setId(taskID);
    clientPSITask->setId(taskID);
    testPSI(factory, serverPSI, clientPSI, serverPSITask, clientPSITask, _expectPSISuccess,
        _expectedPSIResult, _expectedErrorCode);
    std::cout << "### test psi-delete success" << std::endl;
}

void testNormalCase(std::string const& _outputPrefix, CuckoofilterOption::Ptr option,
    uint64_t _dataBatchSize = 10000)
{
    std::string dataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/fullevaluate.txt";
    std::string psiPath = "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/psi.txt";
    std::string appendPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/append-full-evaluate.txt";
    std::string outputPath = _outputPrefix + "psiResult.txt";
    std::vector<std::string> expectedResult;
    for (int i = 1; i < 49; i++)
    {
        expectedResult.emplace_back(std::to_string(i));
    }
    testRA2018PSIImplFunc(_dataBatchSize, option, dataPath, psiPath, outputPath, true, true,
        expectedResult, 0, false, appendPath, std::vector<std::string>());

    // with psi2.txt as input
    psiPath = "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/psi2.txt";
    expectedResult.clear();
    for (int i = 2000; i < 4000; i += 3)
    {
        expectedResult.emplace_back(std::to_string(i));
    }
    outputPath = _outputPrefix + "psiResult2.txt";
    std::vector<std::string> appendIntersectionData;
    std::string prefix = "abcdefg";
    for (int i = 0; i < 4; i++)
    {
        appendIntersectionData.emplace_back(prefix + std::to_string(i));
    }
    appendIntersectionData.emplace_back("abcdefg0");
    testRA2018PSIImplFunc(_dataBatchSize, option, dataPath, psiPath, outputPath, true, true,
        expectedResult, 0, false, appendPath, appendIntersectionData);
}
// the cuckoo-filter not hitted in the server
BOOST_AUTO_TEST_CASE(testMisMatchResourceIDCase)
{
    auto option = std::make_shared<CuckoofilterOption>();
    option->tagBits = 32;
    option->maxKickOutCount = 10;
    option->capacity = 200000;
    std::string dataPath =
        "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/fullevaluate.txt";
    std::string psiPath = "../../../../wedpr-computing/ppc-psi/tests/ra2018-psi/mock-data/psi.txt";
    std::string outputPath = "psiResult_MismatchResource.txt";
    std::vector<std::string> expectedResult;
    testRA2018PSIImplFunc(10000, option, dataPath, psiPath, outputPath, true, false, expectedResult,
        (int)PSIRetCode::NotOfflineFullEvaluated, true);
}

BOOST_AUTO_TEST_CASE(testNormalRA2018PSICase)
{
    // singleCuckooFilter
    auto option = std::make_shared<CuckoofilterOption>();
    option->tagBits = 32;
    option->maxKickOutCount = 10;
    option->capacity = 200000;
    testNormalCase("singleCuckooFilter-", option);

    // with data-batch-size = 100
    testNormalCase("singleCuckooFilter-100-dataBatch-", option, 100);
}

// test normal case with multiple-cuckoo-filter
BOOST_AUTO_TEST_CASE(testNormalRA2018PSICaseWithMultipleCuckooFilter)
{
    auto option = std::make_shared<CuckoofilterOption>();
    option->tagBits = 32;
    option->maxKickOutCount = 100;
    option->capacity = 2000;
    testNormalCase("multipleCuckooFilter-", option);

    // with data-batch-size = 100
    testNormalCase("multipleCuckooFilter-100-dataBatch-", option, 100);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test