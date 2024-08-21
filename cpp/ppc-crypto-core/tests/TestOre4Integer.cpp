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
 * @file TestOre4Integer.cpp
 * @author: shawnhe
 * @date 2023-11-22
 */
#pragma execution_character_set("utf-8")
#include "ppc-crypto-core/src/ore/FastOre.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

using namespace ppc::protocol;
using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(Ore4IntegerTest, TestPromptFixture)

int64_t generateRandomInt64()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int64_t> distribution(-(1LL << 62) + 1, (1LL << 62) - 1);
    return distribution(gen);
}

std::vector<int64_t> generateRandomNumber(int vectorSize)
{
    std::vector<int64_t> numberVector;
    numberVector.reserve(vectorSize);

    for (int i = 0; i < vectorSize; ++i)
    {
        numberVector.push_back(generateRandomInt64());
    }

    // Sort the vector of strings
    std::sort(numberVector.begin(), numberVector.end());
    return numberVector;
}


void runOre(bcos::bytes const& key, FastOre::Ptr _ore, const int64_t& _input)
{
    auto cipher = _ore->encrypt4Integer(bcos::ref(key), _input);
    auto plain = _ore->decrypt4Integer(bcos::ref(key), cipher);
    BOOST_CHECK(_input == plain);
}

void testOreFunctionImpl(bcos::bytes const& key)
{
    auto fastOre = std::make_shared<FastOre>();
    runOre(key, fastOre, -(1LL << 62) + 1);
    runOre(key, fastOre, (1LL << 62) - 1);
    runOre(key, fastOre, 0);
    runOre(key, fastOre, 123456);
    runOre(key, fastOre, -234567);

    auto inputs = generateRandomNumber(1000);
    for (auto input : inputs)
    {
        runOre(key, fastOre, input);
    }
}

BOOST_AUTO_TEST_CASE(testOreFunction)
{
    auto ore = std::make_shared<FastOre>();
    int64_t _plain = -123456;
    OutputBuffer plain{(bcos::byte*)&_plain, sizeof(_plain)};
    ore->formatNumberPlain(&plain, _plain);
    BOOST_CHECK(ore->recoverNumberPlain(plain) == -123456);

    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    testOreFunctionImpl(key);
    key = ore->generateKey();
    testOreFunctionImpl(key);
}

void runOreOrderTest(bcos::bytes const& key, const std::vector<int64_t>& inputs)
{
    auto fastOre = std::make_shared<FastOre>();
    auto preCipher = fastOre->encrypt4Integer(bcos::ref(key), inputs[0]);

    for (uint64_t i = 1; i < inputs.size(); i++)
    {
        auto cipher = fastOre->encrypt4Integer(bcos::ref(key), inputs[i]);
        BOOST_CHECK(fastOre->compare(preCipher, cipher) <= 0);
        preCipher = cipher;
        auto plain = fastOre->decrypt4Integer(bcos::ref(key), cipher);
        BOOST_CHECK(plain == inputs[i]);
    }
}

void testOreOrderImpl(bcos::bytes const& key)
{
    auto inputs = generateRandomNumber(1000);
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

void testEnc(FastOre::Ptr _ore, int _count)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    auto start = bcos::utcTimeUs();
    int64_t input = 712125678;
    for (auto i = 0; i < _count; i++)
    {
        _ore->encrypt4Integer(bcos::ref(key), input);
    }
    std::cout << "Enc, Costs: " << (bcos::utcTimeUs() - start) * 1000 / _count << "ns" << std::endl;
}

void testDec(FastOre::Ptr _ore, int _count)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    int64_t input = 8712125678;
    auto cipher = _ore->encrypt4Integer(bcos::ref(key), input);
    auto start = bcos::utcTimeUs();

    for (auto i = 0; i < _count; i++)
    {
        _ore->decrypt4Integer(bcos::ref(key), cipher);
    }
    std::cout << "Dec, Costs: " << (bcos::utcTimeUs() - start) * 1000 / _count << "ns" << std::endl;
}

void testCompare(bcos::bytes const& key, FastOre::Ptr _ore, int _count)
{
    auto cipher1 = _ore->encrypt4Integer(bcos::ref(key), 8712125678);
    auto cipher2 = _ore->encrypt4Integer(bcos::ref(key), 8712125679);
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
