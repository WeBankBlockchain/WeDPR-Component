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
 * @file TestFastOre.cpp
 * @author: shawnhe
 * @date 2023-11-24
 */

#include "ppc-crypto-c-sdk/fast_ore.h"
#include "ppc-framework/Common.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::test;
namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(fastOreTest, TestPromptFixture)


void testStringOreImpl(bcos::bytes const& plain, bool hex)
{
    // generate sk
    bcos::bytes skBytes(fast_ore_key_bytes());
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    fast_ore_generate_key(&skBuffer);

    // encrypt
    InputBuffer sk{skBytes.data(), skBytes.size()};
    InputBuffer plainBuffer{plain.data(), plain.size()};

    bcos::bytes cipher(fast_ore_get_cipher_size(plain.size(), hex));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    string_fast_ore_encrypt(&cipherBuffer, &sk, &plainBuffer, hex);
    // decrypt
    InputBuffer cipherBuffer2{cipher.data(), cipher.size()};
    bcos::bytes decryptedPlain(fast_ore_get_plain_size(cipher.size(), hex));
    OutputBuffer decryptedPlainBuffer{decryptedPlain.data(), decryptedPlain.size()};
    string_fast_ore_decrypt(&decryptedPlainBuffer, &sk, &cipherBuffer2, hex);
    std::cout << "##### plain: " << std::string(plain.begin(), plain.end()) << std::endl;
    std::cout << "##### decryptedPlain: "
              << std::string(decryptedPlain.begin(), decryptedPlain.end()) << std::endl;
    // check
    BOOST_CHECK(decryptedPlain == plain);
}

void testIntegerOreImpl(const int64_t& plain, bool hex)
{
    // generate sk
    bcos::bytes skBytes(fast_ore_key_bytes());
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    fast_ore_generate_key(&skBuffer);

    // encrypt
    InputBuffer sk{skBytes.data(), skBytes.size()};

    bcos::bytes cipher(fast_ore_get_cipher_size(sizeof(plain), hex));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    integer_fast_ore_encrypt(&cipherBuffer, &sk, plain, hex);
    // decrypt
    InputBuffer cipherBuffer2{cipher.data(), cipher.size()};

    int64_t decryptedPlain;
    integer_fast_ore_decrypt(&decryptedPlain, &sk, &cipherBuffer2, hex);

    // check
    BOOST_CHECK(decryptedPlain == plain);
}

void testFloatOreImpl(const float50& plain, bool hex)
{
    // generate sk
    bcos::bytes skBytes(fast_ore_key_bytes());
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    fast_ore_generate_key(&skBuffer);

    // encrypt
    InputBuffer sk{skBytes.data(), skBytes.size()};

    auto str = plain.str();
    bcos::bytes cipher(fast_ore_get_float_cipher_size(str.size(), hex));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};


    InputBuffer inputBuffer{(bcos::byte*)str.data(), str.size()};
    float_fast_ore_encrypt(&cipherBuffer, &sk, &inputBuffer, hex);

    // decrypt
    InputBuffer cipherBuffer2{cipherBuffer.data, cipherBuffer.len};

    std::string decryptedPlain;
    decryptedPlain.resize(32);
    OutputBuffer decryptedPlainBuffer{(bcos::byte*)decryptedPlain.data(), decryptedPlain.size()};
    float_fast_ore_decrypt(&decryptedPlainBuffer, &sk, &cipherBuffer2, hex);
    decryptedPlain.resize(decryptedPlainBuffer.len);

    // check
    BOOST_CHECK(float50{decryptedPlain} == plain);
}

BOOST_AUTO_TEST_CASE(testStringOreOps)
{
    std::string plain = "adbwerwerwe";
    bcos::bytes plainBytes(plain.begin(), plain.end());
    testStringOreImpl(plainBytes, true);
    testStringOreImpl(plainBytes, false);

    plain = "中文中文";
    bcos::bytes plainBytes2(plain.begin(), plain.end());
    testStringOreImpl(plainBytes2, true);
    testStringOreImpl(plainBytes2, false);
}

BOOST_AUTO_TEST_CASE(testIntegerOreOps)
{
    for (int i = 0; i < 1000; i++)
    {
        testIntegerOreImpl(i * 123456, true);
        testIntegerOreImpl(i * 123456, false);
        testIntegerOreImpl(i * -123456, true);
        testIntegerOreImpl(i * -123456, false);
    }
}

BOOST_AUTO_TEST_CASE(testFloatOreOps)
{
    for (int i = 0; i < 1000; i++)
    {
        auto plain = float50{"-134335849.23449798"};
        testFloatOreImpl(plain * i, true);
        testFloatOreImpl(plain * i, false);
        testFloatOreImpl(plain * -i, true);
        testFloatOreImpl(plain * -i, false);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test