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
 * @file PaillierSumUDFTest.cpp
 * @author: yujiechen
 * @date 2023-08-22
 */
#include "openssl/bn.h"
#include "ppc-crypto-c-sdk/floating_point_paillier.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include "ppc-udf/mysql/paillier_sum.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::test;
using namespace ppc;
using namespace ppc::homo;
using namespace ppc::crypto;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(paillierSumUDFTest, TestPromptFixture)

std::pair<UDF_INIT*, UDF_ARGS*> fakeMySQLParameters(
    InputBuffer const* cipherBytes, InputBuffer const* pkBytes)
{
    UDF_INIT* initid = new UDF_INIT();
    UDF_ARGS* args = new UDF_ARGS();
    args->arg_count = 2;
    args->args = new char*[2];
    if (cipherBytes)
    {
        args->args[0] = (char*)cipherBytes->data;
    }
    args->args[1] = (char*)pkBytes->data;
    args->lengths = new unsigned long[2];
    if (cipherBytes)
    {
        args->lengths[0] = cipherBytes->len;
    }
    args->lengths[1] = pkBytes->len;
    return std::make_pair(initid, args);
}

void testPaillierSum(
    std::vector<int64_t> const& values, InputBuffer const* pkBytes, int keyBits, void* keypair)
{
    int i = 0;
    char* message = new char[1000];
    char* error = new char[1000];
    memset(message, 0, 1000);
    memset(error, 0, 1000);
    float50 expectedSum = 0;
    auto fakedArgs = fakeMySQLParameters(nullptr, pkBytes);
    auto codec = std::make_shared<FloatingPointCodec>();
    for (auto it : values)
    {
        expectedSum += it;
        bcos::bytes cipherData(floating_point_paillier_cipher_bytes(keyBits), 0);
        OutputBuffer cipherBuffer{cipherData.data(), cipherData.size()};

        auto ffpNumber = codec->toFloatingPoint(it);
        BN_print_fp(stdout, ffpNumber.value.bn().get());
        floating_point_paillier_encrypt_fast(
            &cipherBuffer, ffpNumber.value.bn().get(), ffpNumber.exponent, keypair);

        InputBuffer cipherBytes{cipherData.data(), cipherBuffer.len};
        // convert cipherBytes to hex
        auto hexCipher = *(bcos::toHexString(cipherData));
        fakedArgs.second->args[0] = (char*)hexCipher.data();
        fakedArgs.second->lengths[0] = hexCipher.size();
        // init for the fist element
        if (i == 0)
        {
            auto ret = paillier_sum_init(fakedArgs.first, fakedArgs.second, message);
            // init failed
            if (ret)
            {
                break;
            }
        }
        // add for every element
        paillier_sum_add(fakedArgs.first, fakedArgs.second, nullptr, error);
        i++;
    }
    // obtain the result
    char* result = new char[fakedArgs.first->max_length];
    unsigned long length;
    auto retResult =
        paillier_sum(fakedArgs.first, fakedArgs.second, result, &length, nullptr, error);
    // check the result
    std::string hexCipherSum = std::string(retResult, retResult + length);

    bcos::bytes bytesCipherSum = *(bcos::fromHexString(hexCipherSum));
    InputBuffer sumCipherBuffer{bytesCipherSum.data(), bytesCipherSum.size()};
    BigNum decryptV;
    int16_t exponent;
    floating_point_paillier_decrypt(decryptV.bn().get(), &exponent, &sumCipherBuffer, keypair);
    if (get_last_error())
    {
        std::cout << "#### floating_point_paillier_decrypt error: " << get_last_error_msg()
                  << std::endl;
    }
    BOOST_CHECK(get_last_error() == 0);
    FloatingPointNumber floatingPointNumber(std::move(decryptV), exponent);
    BN_print_fp(stdout, floatingPointNumber.value.bn().get());
    auto decryptedResult = codec->toFloat50(floatingPointNumber);
    BOOST_CHECK(decryptedResult == expectedSum);

    paillier_sum_clear(fakedArgs.first, NULL, NULL);
    paillier_sum_deinit(fakedArgs.first);
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

void testPaillierSumUDFImpl(int iter)
{
    srand(bcos::utcSteadyTime());
    int64_t m = 0;
    std::vector<int64_t> values(iter);
    for (int64_t i = 0; i < iter; i++)
    {
        m = 12323434 + rand();
        if (i % 3 == 0)
        {
            m = -1234324 - rand();
        }
        values[i] = m;
    }
    int keyBits = 2048;
    auto keypair = paillier_generate_keypair(keyBits);

    bcos::bytes pkBytes(paillier_max_public_key_bytes(keyBits), 0);
    OutputBuffer pkBuffer{pkBytes.data(), pkBytes.size()};
    paillier_set_public_key_bytes_from_keypair(&pkBuffer, keypair);

    // convert to hexedPk
    auto hexPk = *(bcos::toHexString(pkBytes));
    InputBuffer pkBytesBuffer{(const unsigned char*)hexPk.data(), hexPk.size()};
    testPaillierSum(values, &pkBytesBuffer, keyBits, keypair);
}

BOOST_AUTO_TEST_CASE(testPaillierSumUDF)
{
    testPaillierSumUDFImpl(1);
    testPaillierSumUDFImpl(2);
    testPaillierSumUDFImpl(3);
    testPaillierSumUDFImpl(10);
}

BOOST_AUTO_TEST_CASE(testInvalidPaillierSum)
{
    std::string pk = "234324";
    std::string cipher = "234234";
    InputBuffer cipherBuffer{(const unsigned char*)cipher.data(), cipher.size()};
    InputBuffer pkBuffer{(const unsigned char*)pk.data(), pk.size()};
    auto ret = fakeMySQLParameters(&cipherBuffer, &pkBuffer);
    char* message = new char[1000];
    paillier_sum_init(ret.first, ret.second, message);
    if (ret.first)
    {
        delete ret.first;
    }
    if (ret.second)
    {
        delete ret.second;
    }
    if (message)
    {
        delete[] message;
    }
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test