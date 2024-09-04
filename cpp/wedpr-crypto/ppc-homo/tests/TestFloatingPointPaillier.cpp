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
 * @file TestFloatingPointPaillier.cpp
 * @author: yujiechen
 * @date 2023-08-29
 */
#include "ppc-homo/codec/FloatingPointCodec.h"
#include "ppc-homo/paillier/FloatingPointPaillier.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::homo;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(floatingPointPaillierTest, TestPromptFixture)

void testPaillierHomoProperty(int _keyBits, std::string const& s1, std::string const& s2,
    std::string const& s3, double epsilon)
{
    float50 m1(s1);
    float50 m2(s2);
    float50 v(s3);
    auto codec = std::make_shared<FloatingPointCodec>();
    auto m1Fp = codec->toFloatingPoint(s1);
    auto m2Fp = codec->toFloatingPoint(s2);
    auto vFp = codec->toFloatingPoint(s3);
    std::cout << "* exp for m1: " << m1Fp.exponent << ", value: " << m1Fp.value.getWord()
              << std::endl;
    std::cout << "* exp for m2: " << m2Fp.exponent << ", value: " << m2Fp.value.getWord()
              << std::endl;

    auto paillierImpl = std::make_shared<OpenSSLPaillier>();
    auto fpPaillier = std::make_shared<FloatingPointPaillier>(paillierImpl);
    // generate keypair
    auto keyPair = paillierImpl->generateKeyPair(_keyBits);
    std::cout << "===== encrypt/decrypt check ======" << std::endl;
    // encrypt m1
    bcos::bytes c1(FloatingPointPaillier::maxCipherBytesLen(_keyBits), 0);
    OutputBuffer c1Buffer{c1.data(), c1.size()};
    fpPaillier->encryptFast(&c1Buffer, m1Fp, (void*)keyPair.get());
    // decrypt c1
    auto decrypted_m1 = fpPaillier->decrypt(ref(c1), (void*)keyPair.get());
    // check m1
    BOOST_CHECK(fabs(codec->toFloat50(decrypted_m1) - m1) <= epsilon);

    // encrypt m2
    bcos::bytes c2(FloatingPointPaillier::maxCipherBytesLen(_keyBits), 0);
    OutputBuffer c2Buffer{c2.data(), c2.size()};
    fpPaillier->encryptFast(&c2Buffer, m2Fp, (void*)keyPair.get());
    // decrypt c2
    auto decrypted_m2 = fpPaillier->decrypt(ref(c2), (void*)keyPair.get());
    // check m2
    std::cout << std::fixed << std::setprecision(11);
    std::cout << "#### decrypted_m2: " << codec->toFloat50(decrypted_m2) << std::endl;
    std::cout << "#### m2: " << m2 << std::endl;
    BOOST_CHECK(fabs(codec->toFloat50(decrypted_m2) - m2) <= epsilon);
    std::cout << "===== encrypt/decrypt check done======" << std::endl;

    std::cout << "===== paillier add check ======" << std::endl;
    bcos::bytes addResult(FloatingPointPaillier::maxCipherBytesLen(_keyBits), 0);
    OutputBuffer addResultBuffer{addResult.data(), addResult.size()};
    auto pk = keyPair->pk();
    fpPaillier->add(&addResultBuffer, ref(c1), ref(c2), pk);
    // decrypt and check
    auto decryptAddResult = fpPaillier->decrypt(ref(addResult), (void*)keyPair.get());
    auto expectedAddResult = m1 + m2;

    std::cout << std::fixed << std::setprecision(15);
    std::cout << "#### expectedAddResult: " << expectedAddResult << std::endl;
    std::cout << "#### decryptAddResult: " << codec->toFloat50(decryptAddResult) << std::endl;
    std::cout << "### fabs: " << fabs(codec->toFloat50(decryptAddResult) - expectedAddResult)
              << std::endl;
    BOOST_CHECK(fabs(codec->toFloat50(decryptAddResult) - expectedAddResult) <= epsilon);
    std::cout << "===== paillier add check done======" << std::endl;

    std::cout << "===== paillier sub check ======" << std::endl;
    bcos::bytes subResult(FloatingPointPaillier::maxCipherBytesLen(_keyBits), 0);
    OutputBuffer subResultBuffer{subResult.data(), subResult.size()};
    fpPaillier->sub(&subResultBuffer, ref(c1), ref(c2), pk);
    // decrypt and check
    auto decryptSubResult = fpPaillier->decrypt(ref(subResult), (void*)keyPair.get());
    auto expectedSubResult = m1 - m2;

    std::cout << std::fixed << std::setprecision(15);
    std::cout << "#### expectedSubResult: " << expectedSubResult << std::endl;
    std::cout << "#### decryptSubResult: " << codec->toFloat50(decryptSubResult) << std::endl;
    std::cout << "### fabs: " << fabs(codec->toFloat50(decryptSubResult) - expectedSubResult)
              << std::endl;
    BOOST_CHECK(fabs(codec->toFloat50(decryptSubResult) - expectedSubResult) <= epsilon);
    std::cout << "===== paillier sub check done======" << std::endl;


    std::cout << "===== paillier scalaMul check ======" << std::endl;
    bcos::bytes mulResult(FloatingPointPaillier::maxCipherBytesLen(_keyBits), 0);
    OutputBuffer mulResultBuffer{mulResult.data(), mulResult.size()};
    fpPaillier->scalaMul(&mulResultBuffer, vFp, ref(c1), pk);
    // decrypt and check
    auto decryptMulResult = fpPaillier->decrypt(ref(mulResult), (void*)keyPair.get());
    auto expectedMulResult = v * m1;

    std::cout << std::fixed << std::setprecision(15);
    std::cout << "#### expectedMulResult: " << expectedMulResult << std::endl;
    std::cout << "#### decryptMulResult: " << codec->toFloat50(decryptMulResult) << std::endl;
    std::cout << "### fabs: " << fabs(codec->toFloat50(decryptMulResult) - expectedMulResult)
              << std::endl;
    BOOST_CHECK(fabs(codec->toFloat50(decryptMulResult) - expectedMulResult) <= epsilon);
    std::cout << "===== paillier scalaMul check done======" << std::endl;

    // test the overflow case
    std::cout << "====== test the overflow case ======" << std::endl;
    FloatingPointNumber fpN;
    fpN.value = ((PaillierPublicKey*)pk)->n;
    fpN.exponent = 0;
    bcos::bytes resultBytes(FloatingPointPaillier::maxCipherBytesLen(_keyBits));
    OutputBuffer result{resultBytes.data(), resultBytes.size()};
    BOOST_CHECK_THROW(fpPaillier->encrypt(&result, fpN, (void*)pk), SignedNumberCodecException);
    std::cout << "====== test the overflow case done======" << std::endl;
}

BOOST_AUTO_TEST_CASE(testFloatingPointPaillier)
{
    int keyBits = 2048;
    std::string m1("2.323");
    std::string m2("4.5");
    std::string v("123.23");
    // set precison to 10
    double epsilon = 0.0000000001;
    std::cout << "===== testFloatingPointPaillier: positive case =========" << std::endl;
    testPaillierHomoProperty(keyBits, m1, m2, v, epsilon);
    std::cout << "===== testFloatingPointPaillier: positive case done =========" << std::endl;

    // the negative case
    std::cout << "===== testFloatingPointPaillier: negative case =========" << std::endl;
    m1 = "-23.2344234";
    m2 = "4.234";
    v = "-2343.23434324324";
    testPaillierHomoProperty(keyBits, m1, m2, v, epsilon);
    std::cout << "===== testFloatingPointPaillier: negative case done=========" << std::endl;
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test