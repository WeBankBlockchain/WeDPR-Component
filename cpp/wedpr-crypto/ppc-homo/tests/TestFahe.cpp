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
 * @file TestFahe.cpp
 * @author: yujiechen
 * @date 2023-08-30
 */
#include "ppc-homo/fahe/Fahe.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::homo;
using namespace bcos;
using namespace bcos::test;
namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(faheTest, TestPromptFixture)
void testFaheImpl(BigNum const& m1, BigNum const& m2, int lambda, int mmax, int alpha)
{
    auto faheImpl = std::make_shared<Fahe>();
    auto key = faheImpl->generateKey(lambda, mmax, alpha);

    // test encrypt/decrypt
    auto c1 = faheImpl->encrypt(m1.bn().get(), key.get());
    bcos::bytes cipherBytes;
    c1.toBytes(cipherBytes);
    std::cout << "### cipherLen: " << cipherBytes.size() << std::endl;
    auto decryptedM1 = faheImpl->decrypt(c1.bn().get(), key.get());
    std::cout << "### m1: ";
    BN_print_fp(stdout, m1.bn().get());
    std::cout << std::endl;
    std::cout << "### decryptedM1: ";
    BN_print_fp(stdout, decryptedM1.bn().get());
    std::cout << std::endl;

    BOOST_CHECK(decryptedM1.cmp(m1.bn().get()) == 0);

    auto c2 = faheImpl->encrypt(m2.bn().get(), key.get());
    auto decryptedM2 = faheImpl->decrypt(c2.bn().get(), key.get());

    std::cout << "### m2: ";
    BN_print_fp(stdout, m2.bn().get());
    std::cout << std::endl;
    std::cout << "### decryptedM2: ";
    BN_print_fp(stdout, decryptedM2.bn().get());
    std::cout << std::endl;

    BOOST_CHECK(decryptedM2.cmp(m2.bn().get()) == 0);

    // add
    auto addResult = faheImpl->add(c1.bn().get(), c2.bn().get());
    auto decryptAddResult = faheImpl->decrypt(addResult.bn().get(), key.get());
    auto expectedAddResult = m1.add(m2.bn().get());

    std::cout << "### decryptAddResult: ";
    BN_print_fp(stdout, decryptAddResult.bn().get());
    std::cout << std::endl;
    std::cout << "### expectedAddResult: ";
    BN_print_fp(stdout, expectedAddResult.bn().get());
    std::cout << std::endl;
    BOOST_CHECK(decryptAddResult.cmp(expectedAddResult.bn().get()) == 0);
}

BOOST_AUTO_TEST_CASE(testFahe)
{
    int lambda = 128;
    int alpha = 32;
    int mmax = 64;

    // positive case
    int64_t m1 = 123213231;
    int64_t m2 = 234343430;
    std::cout << "==== testFahe positive =====" << std::endl;
    testFaheImpl(BigNum(m1), BigNum(m2), lambda, mmax, alpha);
    std::cout << "==== testFahe positive finished=====" << std::endl;

    // zero case
    std::cout << "==== testFahe zero case =====" << std::endl;
    m2 = 0;
    testFaheImpl(BigNum(m1), BigNum(m2), lambda, mmax, alpha);
    m1 = 0;
    testFaheImpl(BigNum(m1), BigNum(m2), lambda, mmax, alpha);
    std::cout << "==== testFahe zero case finished =====" << std::endl;

    // negative case
    std::cout << "==== testFahe negative case =====" << std::endl;
    m1 = 123213231 + bcos::utcSteadyTime();
    srand(bcos::utcSteadyTime());
    m2 = -(123213231 + bcos::utcSteadyTime() + rand());
    testFaheImpl(BigNum(m1), BigNum(m2), lambda, mmax, alpha);
    std::cout << "==== testFahe negative case finished =====" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test