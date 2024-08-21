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
 * @file TestIhc.cpp
 * @author: asherli
 * @date 2023-11-27
 */
#include "ppc-homo/ihc/IhcImpl.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::homo;
using namespace bcos;
using namespace bcos::test;
namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(ihcTest, TestPromptFixture)

bcos::bytes testIhcEncryptionDecryption(
    Ihc::Ptr const& _ihc, bcos::bytesConstRef const& key, BigNum const& m)
{
    std::cout << "#### ihc encrypt" << std::endl;
    // encrypt
    auto cipher = _ihc->encrypt(key, m.bn().get());
    std::cout << "#### ihc decrypt" << std::endl;
    // decrypt
    auto decodedM = _ihc->decrypt(key, bcos::ref(cipher));
    std::cout << "#### ihc decrypt success" << std::endl;
    BOOST_CHECK(decodedM.cmp(m.bn().get()) == 0);
    return cipher;
}
void testIhcImpl(BigNum const& m1, BigNum const& m2, BigNum const& m3, Ihc::IhcMode _mode,
    unsigned int _iterRound)
{
    auto ihc = std::make_shared<IhcImpl>((int)_mode, _iterRound);
    auto key = ihc->generateKey();
    // encryption
    auto c1 = testIhcEncryptionDecryption(ihc, bcos::ref(key), m1);
    auto c2 = testIhcEncryptionDecryption(ihc, bcos::ref(key), m2);
    auto c3 = testIhcEncryptionDecryption(ihc, bcos::ref(key), m3);
    // add test
    std::cout << "#### ihc add test" << std::endl;
    auto addResult = ihc->add(bcos::ref(c1), bcos::ref(c2));
    std::cout << "##### addResult: " << bcos::toHex(addResult) << ", size: " << addResult.size()
              << std::endl;
    auto addM = ihc->decrypt(bcos::ref(key), bcos::ref(addResult));
    auto expectedAddM = m1.add(m2.bn().get());
    BOOST_CHECK(addM.cmp(expectedAddM.bn().get()) == 0);
    std::cout << "#### ihc add test success!" << std::endl;
    // sub test
    std::cout << "#### ihc sub test" << std::endl;
    auto subResult = ihc->sub(bcos::ref(c1), bcos::ref(c2));
    auto subM = ihc->decrypt(bcos::ref(key), bcos::ref(subResult));
    auto expectedSubM = m1.sub(m2.bn().get());
    std::cout << "##### subM: " << std::endl;
    BN_print_fp(stdout, subM.bn().get());
    std::cout << "#### expectedSubM: " << std::endl;
    BN_print_fp(stdout, expectedSubM.bn().get());
    BOOST_CHECK(subM.cmp(expectedSubM.bn().get()) == 0);
    std::cout << "#### ihc sub test success" << std::endl;

    // scalaMul test
    std::cout << "#### ihc sub scalaMul" << std::endl;
    auto mulCipher = ihc->scalaMul(m3.bn().get(), bcos::ref(c2));
    auto mulM = ihc->decrypt(bcos::ref(key), bcos::ref(mulCipher));
    auto ctx = createBNContext();
    BigNum expectedMulResult;
    m3.mul(expectedMulResult.bn().get(), m2.bn().get(), ctx.get());
    BOOST_CHECK(mulM.cmp(expectedMulResult.bn().get()) == 0);
    std::cout << "#### ihc sub scalaMul success" << std::endl;
}

BOOST_AUTO_TEST_CASE(testIhc)
{
    int iterRound = 16;

    // positive case
    BigNum m1(123213231);
    BigNum m2(234343430);
    BigNum m3(123456);
    std::cout << "==== testIhc positive =====" << std::endl;
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_128, 16);
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, 16);
    std::cout << "==== testIhc positive finished=====" << std::endl;

    // zero case
    std::cout << "==== testIhc zero case =====" << std::endl;
    m2 = BigNum((int64_t)0);
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_128, 16);
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, 16);
    m1 = BigNum((int64_t)0);
    m3 = BigNum((int64_t)1);
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_128, 16);
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, 16);
    std::cout << "==== testIhc zero case finished =====" << std::endl;

    // negative case
    std::cout << "==== testIhc negative case =====" << std::endl;
    m1 = BigNum(123213231 + bcos::utcSteadyTime());
    srand(bcos::utcSteadyTime());
    m2 = BigNum(-(123213231 + bcos::utcSteadyTime() + rand()));
    m3 = BigNum(122);
    testIhcImpl(m1, m2, m3, Ihc::IhcMode::IHC_256, 16);
    std::cout << "==== testIhc negative case finished =====" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test