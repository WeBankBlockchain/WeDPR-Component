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
 * @file TestPRNG.cpp
 * @author: shawnhe
 * @date 2022-11-29
 */
#include "ppc-crypto-core/src/tools/BitVector.h"
#include "ppc-crypto/src/prng/AESPRNG.h"
#include "ppc-crypto/src/prng/BLAKE2bPRNG.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(PRNGTest, TestPromptFixture)

void runPRNG(PRNG::Ptr _prng1, PRNG::Ptr _prng2)
{
    auto output11 = _prng1->generate(4511);
    auto output12 = _prng1->generate(3275122);
    auto output13 = _prng1->generate(25480233);

    auto output21 = _prng2->generate(4511);
    auto output22 = _prng2->generate(3275122);
    auto output23 = _prng2->generate(25480233);

    BOOST_CHECK(output11 == output21 && output11.size() == 4511);
    BOOST_CHECK(output12 == output22 && output12.size() == 3275122);
    BOOST_CHECK(output13 == output23 && output13.size() == 25480233);
}

void runPRNGFunc(PRNG::Ptr _prng1, PRNG::Ptr _prng2)
{
    auto output11 = _prng1->generate(1234);
    auto output12 = _prng1->generate(23456);
    auto output13 = _prng1->generate(345678);

    auto output2 = _prng2->generate(370368);

    BOOST_CHECK(output11 + output12 + output13 == output2 && output2.size() == 370368);

    int intOut1 = _prng1->generate<int>();
    int intOut2 = _prng2->generate<int>();
    BOOST_CHECK(intOut1 == intOut2);

    bool boolOut1 = _prng1->generate<bool>();
    bool boolOut2 = _prng2->generate<bool>();
    BOOST_CHECK(boolOut1 == boolOut2);

    std::vector<int> dest1(1024);
    std::vector<int> dest2(1024);
    _prng1->generate(dest1.data(), 1024);
    _prng2->generate(dest2.data(), 1024);
    BOOST_CHECK(dest1 == dest2);
}


BOOST_AUTO_TEST_CASE(testAESPRNG)
{
    bcos::bytes seed1{'a', 'b', 'c'};
    auto prng1 = std::make_shared<AESPRNG>(seed1);
    auto prng2 = std::make_shared<AESPRNG>(seed1);
    runPRNG(prng1, prng2);

    bcos::bytes seed2{'c', 'd', '3'};
    prng1 = std::make_shared<AESPRNG>(seed2);
    prng2 = std::make_shared<AESPRNG>(seed2);
    runPRNGFunc(prng1, prng2);
}

BOOST_AUTO_TEST_CASE(testBLAKE2bPRNG)
{
    bcos::bytes seed1{'a', 'b', 'c'};
    auto prng1 = std::make_shared<BLAKE2bPRNG>(seed1);
    auto prng2 = std::make_shared<BLAKE2bPRNG>(seed1);
    runPRNG(prng1, prng2);

    bcos::bytes seed2{'c', 'd', '3'};
    prng1 = std::make_shared<BLAKE2bPRNG>(seed2);
    prng2 = std::make_shared<BLAKE2bPRNG>(seed2);
    runPRNGFunc(prng1, prng2);

    BitVector bv0("10011011101");
    BitVector bv1(10, 12345);
    bcos::bytes seed{'a', 'b', 'c'};
    auto prng0 = std::make_shared<AESPRNG>(seed);
    auto prng4 = std::make_shared<AESPRNG>(seed);
    bv0.randomize(prng0, 16);
    bv1.randomize(prng4, 16);
    BOOST_CHECK(bv1.equals(bv0));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
