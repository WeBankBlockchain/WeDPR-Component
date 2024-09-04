/*
 *  Copyright (C) 2023 WeDPR.
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
 * @file TestFloatingPointCodec.cpp
 * @author: yujiechen
 * @date 2023-08-29
 */
#include "ppc-homo/codec/FloatingPointCodec.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <math.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::homo;
using namespace bcos;
using namespace bcos::test;
namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(floatingPointCodecTest, TestPromptFixture)
void testDoubleCase(FloatingPointNumber const& _number, std::string const& _expectedVStr)
{
    float50 _expectedV(_expectedVStr);
    auto codec = std::make_shared<FloatingPointCodec>();
    auto float50V = codec->toFloat50(_number);
    std::cout << std::fixed << std::setprecision(8);
    std::cout << "float50V: " << float50V << ", expected:" << _expectedV << std::endl;
    std::cout << "epsilon: " << std::numeric_limits<double>::epsilon() << std::endl;
    BOOST_CHECK(fabs(float50V - _expectedV) < std::numeric_limits<double>::epsilon());

    // convert float50V to FloatingPointNumber
    auto convertedNumber = codec->toFloatingPoint(_expectedVStr);
    std::cout << "exp: " << convertedNumber.exponent << std::endl;
    std::cout << "value: " << convertedNumber.value.getWord() << std::endl;

    // encode the convertedNumber again
    auto convertedV = codec->toFloat50(convertedNumber);
    BOOST_CHECK(fabs(convertedV - _expectedV) < std::numeric_limits<double>::epsilon());
}

void testIntCase(FloatingPointNumber const& _number, s1024 const& _expectedV)
{
    // encode
    auto codec = std::make_shared<FloatingPointCodec>();
    auto v = codec->toInt(_number);
    std::cout << "v: " << v << ", _expectedV: " << _expectedV << std::endl;
    BOOST_CHECK(v == _expectedV);

    // decode
    auto convertedNumber = codec->toFloatingPoint(_expectedV);

    // encode again and check
    auto convertedV = codec->toInt(convertedNumber);
    BOOST_CHECK(_expectedV == convertedV);
}

void testBigNumCovert(s1024 _expectedV)
{
    BigNum bn(_expectedV);
    BigNum expectedBn(_expectedV);
    BOOST_CHECK(bn.cmp(expectedBn.bn().get()) == 0);

    // test to S1024
    auto expectedS1024 = bn.toS1024();
    BOOST_CHECK(expectedS1024 == _expectedV);
}

BOOST_AUTO_TEST_CASE(testBigNumConvert)
{
    s1024 value("123243432444440000234234323423424");
    testBigNumCovert(value);

    for (int i = 0; i < 5; i++)
    {
        int64_t expectedV = 123123342343 + utcSteadyTime();
        s1024 value(expectedV);
        testBigNumCovert(value);

        expectedV = -(123123342343 + utcSteadyTime());
        value = s1024(expectedV);
        testBigNumCovert(value);
    }
}

BOOST_AUTO_TEST_CASE(testDouble)
{
    std::cout << "==== test decimal case ===== " << std::endl;
    std::string expectedV("2.23434");
    FloatingPointNumber fpNumber;
    fpNumber.exponent = -5;
    s1024 significant = s1024(223434);
    BigNum significantBn(significant);
    fpNumber.value = significantBn;
    testDoubleCase(fpNumber, expectedV);

    expectedV = "2.2343432423423431";
    fpNumber.exponent = -16;
    significant = s1024(22343432423423431);
    BigNum significantBn2(significant);
    fpNumber.value = significantBn2;
    testDoubleCase(fpNumber, expectedV);


    expectedV = "2234.3432423423431";
    fpNumber.exponent = -13;
    significant = s1024(22343432423423431);
    BigNum significantBn3(significant);
    fpNumber.value = significantBn3;
    testDoubleCase(fpNumber, expectedV);


    expectedV = "2234343242342.3431";
    fpNumber.exponent = -4;
    significant = s1024(22343432423423431);
    BigNum significantBn4(significant);
    fpNumber.value = significantBn4;
    testDoubleCase(fpNumber, expectedV);

    expectedV = "2234343242342343.1";
    fpNumber.exponent = -1;
    significant = s1024(22343432423423431);
    BigNum significantBn5(significant);
    fpNumber.value = significantBn5;
    testDoubleCase(fpNumber, expectedV);

    expectedV = "22343432423423431";
    fpNumber.exponent = 0;
    significant = s1024(22343432423423431);
    BigNum significantBn6(significant);
    fpNumber.value = significantBn6;
    testDoubleCase(fpNumber, expectedV);
    std::cout << "==== test decimal case finished ===== " << std::endl;

    // the positive case
    std::cout << "==== test positive case ===== " << std::endl;
    expectedV = "22343432423100";
    fpNumber.exponent = 2;
    BigNum significantBn7(223434324231);
    fpNumber.value = significantBn7;
    testDoubleCase(fpNumber, expectedV);
    std::cout << "==== test positive case finish ===== " << std::endl;

    std::cout << "==== test zero case ===== " << std::endl;
    // zero
    expectedV = "0";
    fpNumber.exponent = 10;
    BigNum zero(0);
    fpNumber.value = zero;
    testDoubleCase(fpNumber, expectedV);
    std::cout << "==== test zero case finish===== " << std::endl;

    std::cout << "==== test negative case =====" << std::endl;
    expectedV = "-22343432423423431";
    fpNumber.exponent = 0;
    significant = s1024(-22343432423423431);
    BigNum significantBn8(significant);
    fpNumber.value = significantBn8;
    testDoubleCase(fpNumber, expectedV);

    expectedV = "-22343432423423.431";
    fpNumber.exponent = -3;
    significant = s1024(-22343432423423431);
    BigNum significantBn9(significant);
    fpNumber.value = significantBn9;
    testDoubleCase(fpNumber, expectedV);

    expectedV = "-22.343432423423431";
    fpNumber.exponent = -15;
    significant = s1024(-22343432423423431);
    BigNum significantBn10(significant);
    fpNumber.value = significantBn10;
    testDoubleCase(fpNumber, expectedV);
    std::cout << "==== test negative case finish===== " << std::endl;
}
BOOST_AUTO_TEST_CASE(testInt)
{
    std::cout << "==== test int case =====" << std::endl;
    s1024 expectedV = s1024("223434323222342343242342343234300000");
    FloatingPointNumber fpNumber;
    fpNumber.exponent = 5;
    s1024 significant = s1024("2234343232223423432423423432343");
    BigNum tmp(significant);
    fpNumber.value = tmp;
    testIntCase(fpNumber, expectedV);

    expectedV = 200;
    fpNumber.exponent = 2;
    BigNum tmp1(2);
    fpNumber.value = tmp1;
    testIntCase(fpNumber, expectedV);

    expectedV = 2000000000000;
    fpNumber.exponent = -2;
    BigNum tmp2(200000000000000);
    fpNumber.value = tmp2;
    testIntCase(fpNumber, expectedV);

    expectedV = 2000000000001;
    fpNumber.exponent = -2;
    BigNum tmp3(200000000000100);
    fpNumber.value = tmp3;
    testIntCase(fpNumber, expectedV);

    expectedV = 200000000000;
    fpNumber.exponent = -3;
    BigNum tmp4(200000000000100);
    fpNumber.value = tmp4;
    testIntCase(fpNumber, expectedV);

    // Note: not support floor
    expectedV = 200000000000;
    fpNumber.exponent = -3;
    BigNum tmp5(200000000000900);
    fpNumber.value = tmp5;
    testIntCase(fpNumber, expectedV);
    std::cout << "==== test int case finish=====" << std::endl;

    std::cout << "==== test zero case =====" << std::endl;
    expectedV = 0;
    BigNum tmp6(0);
    fpNumber.value = tmp6;
    testIntCase(fpNumber, expectedV);
    std::cout << "==== test zero case finish=====" << std::endl;

    std::cout << "==== test negative case =====" << std::endl;
    expectedV = -200;
    fpNumber.exponent = 2;
    BigNum tmp7(-2);
    fpNumber.value = tmp7;
    testIntCase(fpNumber, expectedV);

    expectedV = -2000000000000;
    fpNumber.exponent = -2;
    BigNum tmp8(-200000000000000);
    fpNumber.value = tmp8;
    testIntCase(fpNumber, expectedV);

    expectedV = -200000000000;
    fpNumber.exponent = -3;
    BigNum tmp9(-200000000000900);
    fpNumber.value = tmp9;
    testIntCase(fpNumber, expectedV);
    std::cout << "==== test negative case finish=====" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test