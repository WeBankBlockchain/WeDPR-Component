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
 * @file PaillierEncryptionUDFTest.cpp
 * @author: caryliao
 * @date 2023-10-24
 */
#include "openssl/bn.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/floating_point_paillier.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-udf/mysql/paillier_enc.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include <random>

using namespace bcos;
using namespace bcos::test;
using namespace ppc;
using namespace ppc::crypto;
using namespace ppc::homo;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(PaillierEncryptionUDFTest, TestPromptFixture)

std::pair<UDF_INIT*, UDF_ARGS*> fakeMySQLParameters(
    InputBuffer const* cipherBytes, InputBuffer const* pkBytes, InputBuffer const* skBytes)
{
    UDF_INIT* initid = new UDF_INIT();
    UDF_ARGS* args = new UDF_ARGS();
    args->arg_count = 2;
    args->args = new char*[args->arg_count];
    args->lengths = new unsigned long[args->arg_count];
    args->arg_type = new Item_result[args->arg_count];
    args->args[1] = (char*)pkBytes->data;
    args->lengths[1] = pkBytes->len;

    args->arg_type[0] = STRING_RESULT;
    args->arg_type[1] = STRING_RESULT;
    return std::make_pair(initid, args);
}

void testPaillierDoubleEnc(std::vector<std::string> const& values, InputBuffer const* pkBytes,
    InputBuffer const* skBytes, int keyBits, void* keypair)
{
    char* message = new char[1000];
    char* error = new char[1000];
    char* result = new char[10000];
    memset(message, 0, 1000);
    memset(error, 0, 1000);
    memset(result, 0, 10000);
    auto fakedArgs = fakeMySQLParameters(nullptr, pkBytes, skBytes);
    std::cout << "#### testPaillierDoubleEnc" << std::endl;
    int i = 0;
    auto codec = std::make_shared<FloatingPointCodec>();
    for (auto it : values)
    {
        // fakedArgs.second->args[0] = (char*)&it;
        const char* strValue = it.c_str();
        fakedArgs.second->args[0] = (char*)strValue;
        fakedArgs.second->lengths[0] = strlen(strValue);
        // init for the fist element
        auto start = std::chrono::high_resolution_clock::now();
        if (i == 0)
        {
            // std::cout << "#### before init" << std::endl;
            auto ret = paillier_enc_init(fakedArgs.first, fakedArgs.second, message);
            // init failed
            if (ret)
            {
                std::cout << "#### init fail" << std::endl;
                break;
            }
            std::cout << "#### init success" << std::endl;
        }
        i++;
        // obtain the result
        unsigned long length = 0;
        auto cipherValue =
            paillier_enc(fakedArgs.first, fakedArgs.second, result, &length, nullptr, error);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        // Print the execution time
        std::cout << " paillier enc execution time: " << duration << " microseconds" << std::endl;
        std::cout << "#### cipherValue: " << cipherValue << std::endl;

        InputBuffer cipherBuffer{(unsigned char*)cipherValue, length};
        BigNum decryptV;
        int16_t exponent;
        floating_point_paillier_decrypt(decryptV.bn().get(), &exponent, &cipherBuffer, keypair);
        if (get_last_error())
        {
            std::cout << "#### floating_point_paillier_decrypt error: " << get_last_error_msg()
                    << std::endl;
        }
        BOOST_CHECK(get_last_error() == 0);
        FloatingPointNumber floatingPointNumber(std::move(decryptV), exponent);
        BN_print_fp(stdout, floatingPointNumber.value.bn().get());
        auto decryptedResult = codec->toFloat50(floatingPointNumber);
        std::cout << "#### decryptedResult: " << decryptedResult << std::endl;
        std::cout << "#### float50(it): " << float50(it) << std::endl;
        // BOOST_CHECK(decryptedResult == float50(it));
    }
    paillier_enc_deinit(fakedArgs.first);
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

void testPaillierEncryptionUDFDoubleImpl(int iter)
{
    srand(bcos::utcSteadyTime());
    std::vector<std::string> values;
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    // for (int i = 0; i < iter; ++i) {
    //     double randomValue = distribution(generator);
    //     values.push_back(randomValue*10000);
    // }
    values.push_back("123.56");
    values.push_back("1000.89");
    values.push_back("2000.76");
    for (const auto& value : values) {
        std::cout << value << " ";
    }
    int keyBits = 2048;
    auto keypair = paillier_generate_keypair(keyBits);

    bcos::bytes pkBytes(paillier_max_public_key_bytes(keyBits), 0);
    bcos::bytes skBytes(paillier_max_private_key_bytes(keyBits), 0);
    OutputBuffer pkBuffer{pkBytes.data(), pkBytes.size()};
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    paillier_set_public_key_bytes_from_keypair(&pkBuffer, keypair);
    paillier_set_private_key_bytes_from_keypair(&skBuffer, keypair);

    // convert to hexedPk
    auto hexPk = *(bcos::toHexString(pkBytes));
    auto hexSk = *(bcos::toHexString(skBytes));
    InputBuffer pkBytesBuffer{(const unsigned char*)hexPk.data(), hexPk.size()};
    InputBuffer skBytesBuffer{(const unsigned char*)hexSk.data(), hexSk.size()};
    std::cout << "#### hexPk:" << pkBytesBuffer.data << std::endl;
    std::cout << "#### hexSk:" << skBytesBuffer.data << std::endl;
    testPaillierDoubleEnc(values, &pkBytesBuffer, &skBytesBuffer, keyBits, keypair);
}

BOOST_AUTO_TEST_CASE(testPaillierEncryptionUDF)
{
    testPaillierEncryptionUDFDoubleImpl(3);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
