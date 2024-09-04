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
 * @file OreEncUDFTest.cpp
 * @author: caryliao
 * @date 2023-11-30
 */
#include "openssl/bn.h"
#include "ppc-crypto-c-sdk/fast_ore.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-udf/mysql/ore_enc.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::test;
using namespace ppc;
using namespace ppc::crypto;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(oreEncUDFTest, TestPromptFixture)

std::pair<UDF_INIT*, UDF_ARGS*> fakeMySQLParameters()
{
    UDF_INIT* initid = new UDF_INIT();
    UDF_ARGS* args = new UDF_ARGS();
    int argCount = 2;
    args->arg_count = argCount;
    args->args = new char*[argCount];
    args->lengths = new unsigned long[argCount];
    args->arg_type = new Item_result[argCount];

    args->arg_type[0] = STRING_RESULT;
    args->arg_type[1] = STRING_RESULT;

    return std::make_pair(initid, args);
}

void testOreEnc(
    InputBuffer const* plainTextBytesBuffer, InputBuffer const* skBytesBuffer)
{
    char* message = new char[1000];
    char* error = new char[1000];
    memset(message, 0, 1000);
    memset(error, 0, 1000);
    auto fakedArgs = fakeMySQLParameters();
    std::cout << "#### testOreEnc" << std::endl;
    bool isHex = true;
    bcos::bytes cipherData(fast_ore_get_cipher_size(plainTextBytesBuffer->len, isHex));
    OutputBuffer cipherBuffer{cipherData.data(), cipherData.size()};
    string_fast_ore_encrypt(&cipherBuffer, skBytesBuffer, plainTextBytesBuffer, isHex);
    // InputBuffer inputBufferCipher{cipherBuffer.data, cipherBuffer.len};

    // convert cipherBytes to hex
    // auto hexCipher = *(bcos::toHexString(cipherData.data(), cipherData.data() + cipherBuffer.len));
    // std::cout << "#### hexCipher: " << hexCipher << std::endl;
    // fakedArgs.second->args[0] = (char*)hexCipher.data();
    // fakedArgs.second->lengths[0] = hexCipher.size();
    fakedArgs.second->args[0] = (char*)plainTextBytesBuffer->data;
    fakedArgs.second->lengths[0] = plainTextBytesBuffer->len; 

    // std::cout << "#### encrypt" << std::endl;
    // bcos::bytes plain(plainTextBytesBuffer->len);
    // OutputBuffer plainBuffer{plain.data(), plain.size()};
    // string_fast_ore_encrypt(&plainBuffer, skBytesBuffer, &inputBufferCipher, isHex);  
    // std::cout << "#### encrypt end: " << plainBuffer.data << "# len: " <<
    // plainBuffer.len << ", size: " << plain.size()<<std::endl;

    auto hexSk =
        *(bcos::toHexString(skBytesBuffer->data, skBytesBuffer->data + skBytesBuffer->len));
    InputBuffer inputBufferHexSk{(const unsigned char*)hexSk.data(), hexSk.size()};

    fakedArgs.second->args[1] = (char*)hexSk.data();
    fakedArgs.second->lengths[1] = hexSk.size();

    std::cout << "args[0]" << fakedArgs.second->args[0] << std::endl;
    std::cout << "lengths[0]" << fakedArgs.second->lengths[0] << std::endl;
    std::cout << "args[1]" << fakedArgs.second->args[1] << std::endl;
    std::cout << "lengths[1]" << fakedArgs.second->lengths[1] << std::endl;

    std::cout << "### before init" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto ret = ore_enc_init(fakedArgs.first, fakedArgs.second, message);
    // init failed
    if (ret)
    {
        std::cout << "#### init fail" << std::endl;
        return;
    }
    std::cout << "#### init success" << std::endl;
    // obtain the result
    char* result = new char[fakedArgs.first->max_length];
    unsigned long length;
    auto encryptV = ore_enc(fakedArgs.first, fakedArgs.second, result, &length, nullptr, error);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Print the execution time
    std::cout << " ore execution time: " << duration << " microseconds" << std::endl;
    auto plainText = plainTextBytesBuffer->data;
    std::cout << "#### ore pV: " << plainText << ", len:" << plainTextBytesBuffer->len << std::endl;
    std::cout << "#### ore dV: " << encryptV << ", len:" << strlen(encryptV) << std::endl;
    BOOST_CHECK(*cipherBuffer.data == *encryptV);
    ore_enc_deinit(fakedArgs.first);
    delete fakedArgs.first;
    delete fakedArgs.second;
    if (message)
    {
        delete[] message;
    }
    if (error)
    {
        delete[] error;
    }
}

void testOreEncUDFImpl()
{
    srand(bcos::utcSteadyTime());
    std::string plainText = "19014527865";
    bcos::bytes plainTextBytes(plainText.begin(), plainText.end());
    InputBuffer plainTextBytesBuffer{
        (const unsigned char*)plainTextBytes.data(), plainTextBytes.size()};

    bcos::bytes skBytes(fast_ore_key_bytes(), 0);
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    fast_ore_generate_key(&skBuffer);


    InputBuffer skBytesBuffer{(const unsigned char*)skBytes.data(), skBytes.size()};
    testOreEnc(&plainTextBytesBuffer, &skBytesBuffer);
}

BOOST_AUTO_TEST_CASE(testOreEncUDF)
{
    testOreEncUDFImpl();
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
