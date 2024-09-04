/**
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
 * @file TestOre4String.cpp
 * @author: shawnhe
 * @date 2023-08-18
 */
#pragma execution_character_set("utf-8")
#include "ppc-crypto-core/src/ore/FastOre.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

using namespace ppc::protocol;
using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(Ore4StringTest, TestPromptFixture)

std::vector<std::string> generateRandomString(int vectorSize, int stringLength)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::vector<std::string> stringVector;
    stringVector.reserve(vectorSize);

    for (int i = 0; i < vectorSize; ++i)
    {
        std::string randomString;
        randomString.reserve(stringLength);

        for (int j = 0; j < stringLength; ++j)
        {
            char randomChar = static_cast<char>(std::rand() % 256);
            randomString.push_back(randomChar);
        }

        stringVector.push_back(randomString);
    }

    // Sort the vector of strings
    std::sort(stringVector.begin(), stringVector.end());
    return stringVector;
}

void runOre(bcos::bytes const& key, Ore::Ptr _ore)
{
    std::string input1 = "123456781234567812345678";
    std::string input2 = "中文中文中文中文";

    auto cipher1 = _ore->encrypt4String(bcos::ref(key), input1);
    auto cipher2 = _ore->encrypt4String(bcos::ref(key), input2);

    auto plain2 = _ore->decrypt4String(bcos::ref(key), cipher2);
    auto plain1 = _ore->decrypt4String(bcos::ref(key), cipher1);
    BOOST_CHECK(input1 == plain1);
    BOOST_CHECK(input2 == plain2);
}

void runOre(bcos::bytes const& key, Ore::Ptr _ore, const std::string& _input)
{
    auto cipher = _ore->encrypt4String(bcos::ref(key), _input);
    auto plain = _ore->decrypt4String(bcos::ref(key), cipher);
    BOOST_CHECK(_input == plain);
}

void testOreFunctionImpl(bcos::bytes const& key)
{
    auto fastOre = std::make_shared<FastOre>();
    runOre(key, fastOre);
    runOre(key, fastOre, "测试生僻字謇鬱齉躞");
    runOre(key, fastOre, "测试标点符号！@#￥%……&*（）~");
    runOre(key, fastOre, "测试中文");
    runOre(key, fastOre, "abcd1234");

    auto inputs = generateRandomString(1000, 24);
    for (auto input : inputs)
    {
        runOre(key, fastOre, input);
    }
}

BOOST_AUTO_TEST_CASE(testOreFunction)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    testOreFunctionImpl(key);

    auto ore = std::make_shared<FastOre>();
    key = ore->generateKey();
    testOreFunctionImpl(key);
}

void runOreOrderTest(bcos::bytes const& key, const std::vector<std::string>& inputs)
{
    auto fastOre = std::make_shared<FastOre>();
    auto preCipher = fastOre->encrypt4String(bcos::ref(key), inputs[0]);

    for (uint64_t i = 1; i < inputs.size(); i++)
    {
        auto cipher = fastOre->encrypt4String(bcos::ref(key), inputs[i]);
        BOOST_CHECK(fastOre->compare(preCipher, cipher) <= 0);
        preCipher = cipher;
        auto plain = fastOre->decrypt4String(bcos::ref(key), cipher);
        BOOST_CHECK(plain == inputs[i]);
    }
}

void testOreOrderImpl(bcos::bytes const& key)
{
    auto inputs = generateRandomString(1000, 24);
    runOreOrderTest(key, inputs);
}

BOOST_AUTO_TEST_CASE(testOreOrder)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    testOreOrderImpl(key);
    auto fastOre = std::make_shared<FastOre>();
    key = fastOre->generateKey();
    testOreOrderImpl(key);
}

void testEnc(Ore::Ptr _ore, int _count)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    auto start = bcos::utcSteadyTime();
    std::string input = "2024-10-29 12:19:40";
    for (auto i = 0; i < _count; i++)
    {
        _ore->encrypt4String(bcos::ref(key), input);
    }
    std::cout << "FastOre Enc, Costs: " << (bcos::utcSteadyTime() - start) << "ms" << std::endl;
    std::cout << "FastOre Enc, us/op: " << (bcos::utcSteadyTime() - start)*1000 / _count << "us" << std::endl;
}

void testDec(Ore::Ptr _ore, int _count)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    std::string input = "2024-10-29 12:19:40";
    auto cipher = _ore->encrypt4String(bcos::ref(key), input);
    auto start = bcos::utcSteadyTime();

    for (auto i = 0; i < _count; i++)
    {
        _ore->decrypt4String(bcos::ref(key), cipher);
    }
    std::cout << "FastOre Dec, Costs: " << (bcos::utcSteadyTime() - start) << "ms" << std::endl;
    std::cout << "FastOre Dec, us/op: " << (bcos::utcSteadyTime() - start)*1000 / _count << "us" << std::endl;
}

void testCompare(bcos::bytes const& key, Ore::Ptr _ore, int _count)
{
    auto cipher1 = _ore->encrypt4String(bcos::ref(key), "500232188712125678");
    auto cipher2 = _ore->encrypt4String(bcos::ref(key), "500232188712125679");
    auto start = bcos::utcTimeUs();

    for (auto i = 0; i < _count; i++)
    {
        _ore->compare(cipher1, cipher2);
    }
    std::cout << "Com, Costs: " << (bcos::utcTimeUs() - start) * 1000 / _count << "ns" << std::endl;
}

BOOST_AUTO_TEST_CASE(PerformanceTest)
{
    auto fastOre = std::make_shared<FastOre>();
    testEnc(fastOre, 100000);
    testDec(fastOre, 100000);
    auto key = fastOre->generateKey();
    testCompare(key, fastOre, 100000);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
