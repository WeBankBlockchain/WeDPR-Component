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
 * @file PaillierDecryptionUDFTest.cpp
 * @author: caryliao
 * @date 2023-10-24
 */
#include "openssl/bn.h"
#include "ppc-crypto-c-sdk/floating_point_paillier.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"
#include "ppc-udf/mysql/paillier_dec.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
#include <random>

using namespace bcos;
using namespace bcos::test;
using namespace ppc;
using namespace ppc::crypto;
using namespace ppc::homo;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(PaillierDecryptionUDFTest, TestPromptFixture)

std::pair<UDF_INIT*, UDF_ARGS*> fakeMySQLParameters(
    InputBuffer const* cipherBytes, InputBuffer const* pkBytes, InputBuffer const* skBytes)
{
    UDF_INIT* initid = new UDF_INIT();
    UDF_ARGS* args = new UDF_ARGS();
    args->arg_count = 4;
    args->args = new char*[4];
    args->lengths = new unsigned long[4];
    args->arg_type = new Item_result[4];
    if (cipherBytes)
    {
        args->args[0] = (char*)cipherBytes->data;
        args->lengths[0] = cipherBytes->len;
    }
    args->args[1] = (char*)pkBytes->data;
    args->lengths[1] = pkBytes->len;
    args->args[2] = (char*)skBytes->data;
    args->lengths[2] = skBytes->len;

    args->arg_type[0] = STRING_RESULT;
    args->arg_type[1] = STRING_RESULT;
    args->arg_type[2] = STRING_RESULT;
    args->arg_type[3] = INT_RESULT;
    return std::make_pair(initid, args);
}

void testPaillierDecInt(std::vector<int> const& values, InputBuffer const* pkBytes,
    InputBuffer const* skBytes, int keyBits, void* keypair)
{
    char* message = new char[1000];
    char* error = new char[1000];
    memset(message, 0, 1000);
    memset(error, 0, 1000);
    auto fakedArgs = fakeMySQLParameters(nullptr, pkBytes, skBytes);
    std::cout << "#### testPaillierDec" << std::endl;
    int i = 0;
    for (auto it : values)
    {
        BigNum v(it);
        bcos::bytes cipherData(floating_point_paillier_cipher_bytes(keyBits), 0);
        OutputBuffer cipherBuffer{cipherData.data(), cipherData.size()};
        int16_t exponent = 0;
        auto _keyPair = (OpenSSLPaillierKeyPair*)keypair;
        auto pk = _keyPair->pk();
        floating_point_paillier_encrypt(&cipherBuffer, v.bn().get(), exponent, pk);
        if (get_last_error() != 0)
        {
            const char* errorMsg = get_last_error_msg();
            std::cout << "#### floating_point_paillier_encrypt error:" << errorMsg << std::endl;
            return;
        }
        InputBuffer cipherBytes{cipherData.data(), cipherData.size()};
        // convert cipherBytes to hex
        auto hexCipher = *(bcos::toHexString(cipherData));
        std::cout << "#### hexCipher: " << hexCipher << std::endl;
        fakedArgs.second->args[0] = (char*)hexCipher.data();
        fakedArgs.second->lengths[0] = hexCipher.size();
        // fakedArgs.second->args[3] = (char*)"0";
        // fakedArgs.second->lengths[3] = 1;
        int scale = 0;
        fakedArgs.second->args[3] = (char*)&scale;
        fakedArgs.second->lengths[3] = sizeof(int*);
        // init for the fist element
        if (i == 0)
        {
            std::cout << "#### before init" << std::endl;
            auto ret = paillier_dec_init(fakedArgs.first, fakedArgs.second, message);
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
        char* result = new char[fakedArgs.first->max_length];
        unsigned long length;
        auto decryptV =
            paillier_dec(fakedArgs.first, fakedArgs.second, result, &length, nullptr, error);
        std::cout << "#### int pV: " << it << std::endl;
        std::cout << "#### int dV: " << decryptV << std::endl;
        BOOST_CHECK(decryptV == it);
    }
    paillier_dec_deinit(fakedArgs.first);
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

void testPaillierDoubleDec(std::vector<std::string> const& values, InputBuffer const* pkBytes,
    InputBuffer const* skBytes, int keyBits, void* keypair)
{
    char* message = new char[1000];
    char* error = new char[1000];
    memset(message, 0, 1000);
    memset(error, 0, 1000);
    auto fakedArgs = fakeMySQLParameters(nullptr, pkBytes, skBytes);
    std::cout << "#### testPaillierDoubleDec" << std::endl;
    int i = 0;
    auto codec = std::make_shared<FloatingPointCodec>();
    for (auto strValue : values)
    {
        auto it = float50(strValue);
        auto fpn = codec->toFloatingPoint(strValue);
        bcos::bytes cipherData(floating_point_paillier_cipher_bytes(keyBits), 0);
        OutputBuffer cipherBuffer{cipherData.data(), cipherData.size()};
        int16_t exponent = fpn.exponent;
        auto _keyPair = (OpenSSLPaillierKeyPair*)keypair;
        auto pk = _keyPair->pk();
        floating_point_paillier_encrypt(&cipherBuffer, fpn.value.bn().get(), exponent, pk);
        if (get_last_error() != 0)
        {
            const char* errorMsg = get_last_error_msg();
            std::cout << "#### floating_point_paillier_encrypt error:" << errorMsg << std::endl;
            return;
        }
        InputBuffer cipherBytes{cipherData.data(), cipherData.size()};
        // convert cipherBytes to hex
        auto hexCipher = *(bcos::toHexString(cipherData));
        std::cout << "#### hexCipher: " << hexCipher << std::endl;
        fakedArgs.second->args[0] = (char*)hexCipher.data();
        fakedArgs.second->lengths[0] = hexCipher.size();
        int scale = 2;
        fakedArgs.second->args[3] = (char*)&scale;
        fakedArgs.second->lengths[3] = sizeof(int*);
        // init for the fist element
        auto start = std::chrono::high_resolution_clock::now();
        if (i == 0)
        {
            // std::cout << "#### before init" << std::endl;
            auto ret = paillier_dec_init(fakedArgs.first, fakedArgs.second, message);
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
        char* result = new char[fakedArgs.first->max_length];
        unsigned long length;
        auto decryptV =
            paillier_dec(fakedArgs.first, fakedArgs.second, result, &length, nullptr, error);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        // Print the execution time
        std::cout << " paillier execution time: " << duration << " microseconds" << std::endl;

        double multiplier = pow(10.0, scale);
        double roundPv = round((double)it * multiplier) / multiplier;
        std::cout << "#### double pV: " << roundPv << std::endl;
        std::cout << "#### double dV: " << decryptV << std::endl;
        BOOST_CHECK(decryptV == roundPv);
    }
    paillier_dec_deinit(fakedArgs.first);
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

void testPaillierDecryptionUDFIntImpl(int iter)
{
    srand(bcos::utcSteadyTime());
    int64_t m = 0;
    std::vector<int> values(iter);
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
    testPaillierDecInt(values, &pkBytesBuffer, &skBytesBuffer, keyBits, keypair);
}

void testPaillierDecryptionUDFDoubleImpl(int iter)
{
    srand(bcos::utcSteadyTime());
    std::vector<std::string> values;
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(-109900234.23432434, 10990023423432434);
    for (int i = 0; i < iter; ++i)
    {
        float50 randomValue = float50(distribution(generator));
        values.push_back(randomValue.str(10));
    }
    for (const auto& value : values)
    {
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
    testPaillierDoubleDec(values, &pkBytesBuffer, &skBytesBuffer, keyBits, keypair);
}

BOOST_AUTO_TEST_CASE(testPaillierDecryptionUDF)
{
    testPaillierDecryptionUDFIntImpl(3);
    testPaillierDecryptionUDFDoubleImpl(3);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
