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
 * @file TestIhcFloating.cpp
 * @author: asherli
 * @date 2023-11-27
 */
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include "ppc-homo/ihc/FloatingPointIhc.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::homo;
using namespace bcos;
using namespace bcos::test;
namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(ihcFloatingTest, TestPromptFixture)

bcos::bytes testEncryptDecrypt(FloatingPointIhc::Ptr const& fpIhc,
    FloatingPointCodec::Ptr const& codec, bcos::bytes const& _key, std::string const& s)
{
    float50 m(s);
    auto ffpNumber = codec->toFloatingPoint(s);
    auto cipher = fpIhc->encrypt(bcos::ref(_key), ffpNumber);
    // decrypt
    auto decryptffp = fpIhc->decrypt(bcos::ref(_key), bcos::ref(cipher));
    auto decryptValue = codec->toFloat50(decryptffp);
    // check
    std::cout << "#### m: " << m.str(32) << std::endl;
    std::cout << "#### decryptValue: " << decryptValue.str(32) << std::endl;
    BOOST_CHECK(m == decryptValue);
    return cipher;
}

void testIhcFloatingImpl(
    std::string s1, std::string s2, std::string s3, Ihc::IhcMode _mode, int iterRound)
{
    auto ihc = std::make_shared<IhcImpl>((int)_mode, iterRound);
    auto key = ihc->generateKey();

    float50 m1(s1);
    float50 m2(s2);
    float50 m3(s3);
    auto fpIhc = std::make_shared<FloatingPointIhc>(ihc);
    auto codec = std::make_shared<FloatingPointCodec>();
    // c1
    auto c1 = testEncryptDecrypt(fpIhc, codec, key, s1);
    // c2
    auto c2 = testEncryptDecrypt(fpIhc, codec, key, s2);
    // c3
    auto c3 = testEncryptDecrypt(fpIhc, codec, key, s3);
    // add
    std::cout << "### testIhcFloatingImpl: add case" << std::endl;
    auto addResult = fpIhc->add(bcos::ref(c1), bcos::ref(c2));
    auto addFfpResult = fpIhc->decrypt(bcos::ref(key), bcos::ref(addResult));
    auto expectedAddResult = m1 + m2;
    BOOST_CHECK(codec->toFloat50(addFfpResult) == expectedAddResult);
    std::cout << "### testIhcFloatingImpl: add case end" << std::endl;
    // sub case
    std::cout << "### testIhcFloatingImpl: sub case" << std::endl;
    auto subResult = fpIhc->sub(bcos::ref(c1), bcos::ref(c2));
    auto subFfpResult = fpIhc->decrypt(bcos::ref(key), bcos::ref(subResult));
    auto expectedSubResult = m1 - m2;
    BOOST_CHECK(codec->toFloat50(subFfpResult) == expectedSubResult);
    std::cout << "### testIhcFloatingImpl: sub case end" << std::endl;
    // scalaMul case
    std::cout << "### testIhcFloatingImpl: scalaMul case" << std::endl;
    auto v = codec->toFloatingPoint(s3);
    auto mulResult = fpIhc->scalaMul(v, bcos::ref(c1));
    auto mulFfpResult = fpIhc->decrypt(bcos::ref(key), bcos::ref(mulResult));
    auto expectedMulResult = m1 * m3;
    std::cout << "#### expectedMulResult: " << expectedMulResult << std::endl;
    std::cout << "#### mulFfpResult: " << codec->toFloat50(mulFfpResult) << std::endl;
    BOOST_CHECK(codec->toFloat50(mulFfpResult) == expectedMulResult);
    std::cout << "### testIhcFloatingImpl: mul case end" << std::endl;
}


BOOST_AUTO_TEST_CASE(testIhcFloating)
{
    int iterRound = 16;

    // positive case
    std::cout << "==== testIhcFloatingImpl positive =====" << std::endl;
    std::string m1("234324.345435345345");
    std::string m2("534252.45670234");
    std::string m3("234.456452");
    testIhcFloatingImpl(m1, m2, m3, Ihc::IhcMode::IHC_128, iterRound);
    testIhcFloatingImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, iterRound);
    std::cout << "==== testIhcFloatingImpl positive finished=====" << std::endl;

    // zero case
    std::cout << "==== testIhcFloatingImpl zero case =====" << std::endl;
    m1 = "0.0";
    m2 = "0.1";
    m3 = "0.000";
    testIhcFloatingImpl(m1, m2, m3, Ihc::IhcMode::IHC_128, iterRound);
    testIhcFloatingImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, iterRound);
    std::cout << "==== testIhc zero case finished =====" << std::endl;

    // negative case
    std::cout << "==== testIhcFloatingImpl negative case =====" << std::endl;
    m1 = "-234234.00234324";
    m2 = "-234.543645";
    m3 = "-23432435234";
    testIhcFloatingImpl(m1, m2, m3, Ihc::IhcMode::IHC_128, iterRound);
    testIhcFloatingImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, iterRound);
    std::cout << "==== testIhcFloatingImpl negative case finished =====" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test