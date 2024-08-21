/*
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
 * @file TestTransTools.h
 * @author: shawnhe
 * @date 2022-11-07
 */

#include "ppc-tools/src/common/TransTools.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::test;
using namespace ppc::tools;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(TransToolsTest, TestPromptFixture)

template <typename T>
void testEncodeAndDecodeFunc(T _number)
{
    auto buffer = std::make_shared<bytes>();
    encodeUnsignedNum(buffer, _number);

    T num;
    decodeUnsignedNum(num, buffer);

    BOOST_CHECK(num == _number);
}

template <typename T>
void testEncodeAndDecodeStrFunc(T _number)
{
    std::string buffer = "";
    encodeUnsignedNum(buffer, _number);
    
    T num;
    decodeUnsignedNum(num, buffer);

    BOOST_CHECK(num == _number);
}

BOOST_AUTO_TEST_CASE(testEncodeAndDecode)
{
    testEncodeAndDecodeFunc(uint8_t(17));
    testEncodeAndDecodeFunc(uint16_t(257));
    testEncodeAndDecodeFunc(uint32_t(65537));

    testEncodeAndDecodeStrFunc(uint8_t(17));
    testEncodeAndDecodeStrFunc(uint16_t(257));
    testEncodeAndDecodeStrFunc(uint32_t(65537));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test