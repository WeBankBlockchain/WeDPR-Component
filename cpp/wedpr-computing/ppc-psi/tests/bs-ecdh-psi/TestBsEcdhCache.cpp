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
 * @file TestBsEcdhCache.cpp
 * @author: shawnhe
 * @date 2023-10-10
 */

#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-psi/src/bs-ecdh-psi/BsEcdhPSIFactory.h"
#include "ppc-psi/src/bs-ecdh-psi/core/BsEcdhCache.h"
#include "test-utils/FakeFront.h"
#include "test-utils/FileTool.h"
#include "test-utils/TaskMock.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

using namespace ppc::psi;
using namespace bcos;
using namespace bcos::test;
using namespace ppc::protocol;
using namespace ppc::io;
using namespace ppc::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(BsEcdhCacheTest, TestPromptFixture)

void testEcdhFunc(const std::string& _input, BsEcdhCache::Ptr _cache0, BsEcdhCache::Ptr _cache1)
{
    _cache0->generateKey();
    _cache1->generateKey();
    auto cipher0 = _cache0->genCipherWithBase64(_input);
    auto cipher1 = _cache1->genCipherWithBase64(_input);
    auto ecdhCipher0 = _cache1->genEcdhCipherWithBase64(cipher0);
    auto ecdhCipher1 = _cache0->genEcdhCipherWithBase64(cipher1);
    BOOST_CHECK(ecdhCipher0 == ecdhCipher1);
    BOOST_CHECK(ecdhCipher0.size() > POINT_SIZE);
    BOOST_CHECK(ecdhCipher1.size() > POINT_SIZE);
}

BOOST_AUTO_TEST_CASE(testEcdhEnc)
{
    auto cache0 = std::make_shared<BsEcdhCache>(
        "test0", nullptr, nullptr, nullptr, false, false, nullptr, nullptr, nullptr, 100);
    auto cache1 = std::make_shared<BsEcdhCache>(
        "test1", nullptr, nullptr, nullptr, false, false, nullptr, nullptr, nullptr, 100);
    for (int i = 0; i < 100; i++)
    {
        testEcdhFunc(std::to_string(i), cache0, cache1);
    }

    std::string input = "hello world";
    std::string key0 = bcos::base64Decode("teT9hMCdq8UCskX6CRlJ9bX7bC5TZ4M1O/eQlpHmyws=");
    std::string key1 = bcos::base64Decode("Oav3jsBuamDwLyUo33VOYumJc0f/ySo5oLbkBe+fMgo=");
    cache0->setKey(bcos::bytes(key0.begin(), key0.end()));
    cache1->setKey(bcos::bytes(key1.begin(), key1.end()));
    auto cipher0 = cache0->genCipherWithBase64(input);
    auto cipher1 = cache1->genCipherWithBase64(input);
    auto ecdhCipher0 = cache1->genEcdhCipherWithBase64(cipher0);
    auto ecdhCipher1 = cache0->genEcdhCipherWithBase64(cipher1);

    std::cout << cipher0 << std::endl;
    std::cout << cipher1 << std::endl;
    std::cout << ecdhCipher0 << std::endl;
    std::cout << ecdhCipher1 << std::endl;

    BOOST_CHECK(ecdhCipher0 == ecdhCipher1);
}

BOOST_AUTO_TEST_CASE(testBsEcdh)
{
    auto inputPath = "../../../../wedpr-computing/ppc-psi/tests/bs-ecdh-psi/data/inputs.csv";
    auto outputPath = "../../../../../wedpr-computing/ppc-psi/tests/bs-ecdh-psi/data/outputs.csv";
    boost::filesystem::create_directory(
        "../../../../wedpr-computing/ppc-psi/tests/bs-ecdh-psi/data");

    auto count = 123456;
    prepareInputs(inputPath, count);

    auto taskRequest = std::make_shared<RunTaskRequest>();
    auto taskID = "0x123456";
    auto dataResource = std::make_shared<DataResource>("id");
    auto dataDesc = std::make_shared<DataResourceDesc>();
    dataDesc->setType(0);
    dataDesc->setPath(inputPath);
    dataResource->setDesc(dataDesc);
    auto outDesc = std::make_shared<DataResourceDesc>();
    outDesc->setType(0);
    outDesc->setPath(outputPath);
    dataResource->setOutputDesc(outDesc);
    taskRequest->dataResource = dataResource;
    taskRequest->taskID = taskID;
    taskRequest->enableAudit = true;

    auto dataResourceLoader = std::make_shared<DataResourceLoaderImpl>(
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    auto bsEcdhPSIFactory = std::make_shared<BsEcdhPSIFactory>();
    auto bsEcdhPSI = bsEcdhPSIFactory->buildBsEcdhPSI(
        std::make_shared<bcos::ThreadPool>("test_bs-ecdh-psi", std::thread::hardware_concurrency()),
        dataResourceLoader, 60);
    bsEcdhPSI->enableOutputExists();
    bsEcdhPSI->start();
    bsEcdhPSI->asyncRunTask(taskRequest, nullptr);

    auto getTaskStatusRequest = std::make_shared<GetTaskStatusRequest>();
    getTaskStatusRequest->taskID = taskID;

    while (bsEcdhPSI->getTaskStatus(getTaskStatusRequest)->data()["status"].asString() ==
           protocol::toString(TaskStatus::PENDING))
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    BOOST_CHECK(bsEcdhPSI->getTaskStatus(getTaskStatusRequest)->data()["status"].asString() ==
                protocol::toString(TaskStatus::RUNNING));

    auto cache = std::make_shared<BsEcdhCache>(
        "test", nullptr, nullptr, nullptr, false, false, nullptr, nullptr, nullptr, count);
    cache->generateKey();

    uint32_t offset = 0;
    uint32_t size = 10000;
    uint32_t total = count;
    do
    {
        auto fetchCipherRequest = std::make_shared<FetchCipherRequest>();
        fetchCipherRequest->taskID = taskID;
        fetchCipherRequest->offset = offset;
        fetchCipherRequest->size = size;
        auto result = bsEcdhPSI->fetchCipher(fetchCipherRequest);
        total = result->data()["total"].asInt();
        std::vector<std::string> ciphers;
        for (const Json::Value& cipher : result->data()["ciphers"])
        {
            ciphers.push_back(cipher.asString());
        }

        auto sendEcdhCipherRequest = std::make_shared<SendEcdhCipherRequest>();
        sendEcdhCipherRequest->taskID = taskID;
        sendEcdhCipherRequest->offset = offset;
        sendEcdhCipherRequest->size = ciphers.size();

        for (const auto& cipher : ciphers)
        {
            sendEcdhCipherRequest->ecdhCiphers.push_back(cache->genEcdhCipherWithBase64(cipher));
        }
        bsEcdhPSI->sendEcdhCipher(sendEcdhCipherRequest);
        bsEcdhPSI->sendEcdhCipher(sendEcdhCipherRequest);

        auto sendPartnerCipherRequest = std::make_shared<SendPartnerCipherRequest>();
        sendPartnerCipherRequest->taskID = taskID;
        sendPartnerCipherRequest->offset = offset;
        sendPartnerCipherRequest->size = ciphers.size();
        sendPartnerCipherRequest->total = total;

        for (uint32_t i = offset; i < ciphers.size() + offset; i++)
        {
            sendPartnerCipherRequest->partnerCiphers.push_back(
                cache->genCipherWithBase64(std::to_string(100000 + i)));
        }
        bsEcdhPSI->sendPartnerCipher(sendPartnerCipherRequest);
        bsEcdhPSI->sendPartnerCipher(sendPartnerCipherRequest);

        offset += ciphers.size();
    } while (offset < total);

    while (bsEcdhPSI->getTaskStatus(getTaskStatusRequest)->data()["status"].asString() ==
           protocol::toString(TaskStatus::RUNNING))
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    BOOST_CHECK(bsEcdhPSI->getTaskStatus(getTaskStatusRequest)->data()["status"].asString() ==
                protocol::toString(TaskStatus::COMPLETED));
}


BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test