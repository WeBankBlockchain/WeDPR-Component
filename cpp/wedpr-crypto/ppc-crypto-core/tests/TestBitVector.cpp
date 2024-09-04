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
 * @file TestBitvector.cpp
 * @author: shawnhe
 * @date 2022-12-6
 */

#include "ppc-crypto-core/src/tools/BitVector.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(BitvectorTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(testBitvector)
{
    auto str = "10011011101";
    BitVector bv(str);
    BOOST_CHECK(str == bv.toString());

    BitVector bv0("10011011101");
    BOOST_CHECK(bv.equals(bv0));

    BitVector bv1(10, 12345);
    BOOST_CHECK("0000111001" == bv1.toString());
    BOOST_CHECK(bv1.get(0) && !bv1.get(1) && bv1.get(3));
    bv1.set(0, 0), bv1.set(1, 1), bv1.set(2, 1);
    BOOST_CHECK("0000111110" == bv1.toString());

    BitVector bv2(bcos::bytes{'a'});
    bv2.append(bcos::bytes{'b'});
    bv2.append(bcos::bytes{'c'});
    BOOST_CHECK("011000110110001001100001" == bv2.toString());
    bcos::bytes bytes1{'c', 'b', 'a'};
    BOOST_CHECK(bytes1 == bv2.toBytes());
    bv2.append("111000111");
    BOOST_CHECK("111000111011000110110001001100001" == bv2.toString());
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
