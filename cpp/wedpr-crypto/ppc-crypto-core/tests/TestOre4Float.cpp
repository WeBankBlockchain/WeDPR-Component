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
 * @file TestOre4Float.cpp
 * @author: shawnhe
 * @date 2023-12-07
 */
#pragma execution_character_set("utf-8")
#include "ppc-crypto-core/src/ore/FastOre.h"
#include "ppc-framework/libwrapper/OreFloatingNumber.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <random>
#include <vector>

using namespace ppc::protocol;
using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;
namespace bmp = boost::multiprecision;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(Ore4FloatTest, TestPromptFixture)

float50 generateRandomFloat50()
{
    std::random_device rd;
    std::mt19937_64 gen0(rd());
    std::uniform_int_distribution<int64_t> distribution0(-(1LL << 62) + 2, (1LL << 62) - 2);
    int64_t integerPart = distribution0(gen0);
    std::mt19937_64 gen1(rd());
    std::uniform_int_distribution<int64_t> distribution1(0, 99999999);
    int64_t decimalPart = distribution1(gen1);
    float50 randomCppFloat(std::to_string(integerPart) + "." + std::to_string(decimalPart));
    return randomCppFloat;
}

std::vector<float50> generateRandomFloatNumbers(int vectorSize)
{
    std::vector<float50> numberVector;
    numberVector.reserve(vectorSize);

    for (int i = 0; i < vectorSize; ++i)
    {
        numberVector.push_back(generateRandomFloat50());
    }

    // Sort the vector of strings
    std::sort(numberVector.begin(), numberVector.end());
    return numberVector;
}

void runOre(bcos::bytes const& key, FastOre::Ptr _ore, const float50& _input)
{
    auto cipher = _ore->encrypt4Float(bcos::ref(key), _input);
    auto plain = _ore->decrypt4Float(bcos::ref(key), cipher);
    BOOST_CHECK(_input == plain);
}

void testOreFunctionImpl(bcos::bytes const& key)
{
    auto fastOre = std::make_shared<FastOre>();
    runOre(key, fastOre, float50{"-4611686018427387901.0000000000000213433437899"});
    runOre(key, fastOre, float50{"4611686018427387901.3437899"});
    runOre(key, fastOre, 0);

    auto inputs = generateRandomFloatNumbers(1000);
    for (auto input : inputs)
    {
        runOre(key, fastOre, input);
    }
}

BOOST_AUTO_TEST_CASE(testOreFunction)
{
    auto ore = std::make_shared<FastOre>();
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    testOreFunctionImpl(key);
    key = ore->generateKey();
    testOreFunctionImpl(key);
}

void runOreOrderTest(bcos::bytes const& key, const std::vector<float50>& inputs)
{
    auto fastOre = std::make_shared<FastOre>();
    auto preCipher = fastOre->encrypt4Float(bcos::ref(key), inputs[0]);

    for (uint64_t i = 1; i < inputs.size(); i++)
    {
        auto cipher = fastOre->encrypt4Float(bcos::ref(key), inputs[i]);
        BOOST_CHECK(fastOre->compare(preCipher, cipher) <= 0);
        preCipher = cipher;
        auto plain = fastOre->decrypt4Float(bcos::ref(key), cipher);
        BOOST_CHECK(plain == inputs[i]);
    }
}

void testOreOrderImpl(bcos::bytes const& key)
{
    auto inputs = generateRandomFloatNumbers(1000);
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
    float50 input("7121925678.23456789");
    for (auto i = 0; i < _count; i++)
    {
        _ore->encrypt4Float(bcos::ref(key), input);
    }
    std::cout << "Enc, Costs: " << (bcos::utcTimeUs() - start) * 1000 / _count << "ns" << std::endl;
}

void testDec(FastOre::Ptr _ore, int _count)
{
    bcos::bytes key{'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd'};
    float50 input("7121925678.23456789");
    auto cipher = _ore->encrypt4Float(bcos::ref(key), input);
    auto start = bcos::utcTimeUs();

    for (auto i = 0; i < _count; i++)
    {
        _ore->decrypt4Float(bcos::ref(key), cipher);
    }
    std::cout << "Dec, Costs: " << (bcos::utcTimeUs() - start) * 1000 / _count << "ns" << std::endl;
}

void testCompare(bcos::bytes const& key, FastOre::Ptr _ore, int _count)
{
    auto cipher1 = _ore->encrypt4Float(bcos::ref(key), float50{"7121925678.23456789"});
    auto cipher2 = _ore->encrypt4Float(bcos::ref(key), float50{"7121925678.23456789"});
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

void testOreFloatingNumber(std::string _value, std::string _target)
{
    float50 cppDecFloatValue(_value);
    OreFloatingNumber ofn(cppDecFloatValue);
    std::cout << "### valueStr: " << ofn.value().str() << ", target: " << _target << std::endl;
    BOOST_CHECK(ofn.value().str() == _target);
}

BOOST_AUTO_TEST_CASE(testOreFloatingNumberFunc)
{
    testOreFloatingNumber("0.00000", "0");
    testOreFloatingNumber("12374435346556789", "12374435346556789");
    testOreFloatingNumber("1233456789.0", "1233456789");
    testOreFloatingNumber("123456789.000000", "123456789");
    testOreFloatingNumber("12364456789.1234500", "12364456789.12345");
    testOreFloatingNumber("1234566789.000000019", "1234566789.000000019");
    testOreFloatingNumber("1234566789.999999999", "1234566789.999999999");
    testOreFloatingNumber("-1237456789", "-1237456789");
    testOreFloatingNumber("-123456789.000000", "-123456789");
    testOreFloatingNumber("-12364456789.1234500", "-12364456789.12345");
    testOreFloatingNumber("-1234566789.12345678999", "-1234566789.12345678999");
    testOreFloatingNumber("-1234566789.00000001", "-1234566789.00000001");
    testOreFloatingNumber("-12374435346556789.99999999", "-12374435346556789.99999999");
    testOreFloatingNumber("-12374435346556789.88888888", "-12374435346556789.88888888");
    testOreFloatingNumber("-1234566789.8888", "-1234566789.8888");

    testOreFloatingNumber(
        "-12374435346556789.000000000000088888888", "-12374435346556789.000000000000088888888");
    testOreFloatingNumber("12374435346556789.0000000000000000000000000000000000011118888888811111",
        "12374435346556789.0000000000000000000000000000000000011118888888811111");
    // precision truncate case
    testOreFloatingNumber(
        "25347862354.0000000000000000234564564500000000000000023434200000000000456456",
        "25347862354.00000000000000002345645645000000000000000234342");
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
