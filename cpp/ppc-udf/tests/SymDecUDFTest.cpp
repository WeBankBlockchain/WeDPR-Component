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
 * @file SymDecUDFTest.cpp
 * @author: caryliao
 * @date 2023-10-25
 */
#include "openssl/bn.h"
#include "ppc-crypto-c-sdk/symmetric_encryption.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-framework/crypto/SymCrypto.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-udf/mysql/sym_dec.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::test;
using namespace ppc;
using namespace ppc::crypto;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(symDecUDFTest, TestPromptFixture)

std::pair<UDF_INIT*, UDF_ARGS*> fakeMySQLParameters(InputBuffer const* cipherBytes,
    int* algorithmType, int* mode, InputBuffer const* ivBytesBuffer,
    InputBuffer const* skBytesBuffer)
{
    UDF_INIT* initid = new UDF_INIT();
    UDF_ARGS* args = new UDF_ARGS();
    args->arg_count = 5;
    args->args = new char*[5];
    args->lengths = new unsigned long[5];
    args->arg_type = new Item_result[5];
    if (cipherBytes)
    {
        args->args[0] = (char*)cipherBytes->data;
        args->lengths[0] = cipherBytes->len;
    }
    args->args[1] = (char*)algorithmType;
    args->lengths[1] = sizeof(int);
    args->args[2] = (char*)mode;
    args->lengths[2] = sizeof(int);

    args->arg_type[0] = STRING_RESULT;
    args->arg_type[1] = INT_RESULT;
    args->arg_type[2] = INT_RESULT;
    args->arg_type[3] = STRING_RESULT;
    args->arg_type[4] = STRING_RESULT;
    return std::make_pair(initid, args);
}

void testSymDec(InputBuffer const* plainTextBytesBuffer, int algorithmType, int mode,
    InputBuffer const* ivBytesBuffer, InputBuffer const* skBytesBuffer)
{
    char* message = new char[1000];
    char* error = new char[1000];
    memset(message, 0, 1000);
    memset(error, 0, 1000);
    BigNum expectedSum(0);
    auto fakedArgs =
        fakeMySQLParameters(nullptr, &algorithmType, &mode, ivBytesBuffer, skBytesBuffer);
    std::cout << "#### testSymDec" << std::endl;
    bcos::bytes cipherData(symmetric_block_size(algorithmType) + plainTextBytesBuffer->len);
    OutputBuffer cipherBuffer{cipherData.data(), cipherData.size()};
    symmetric_encrypt(
        &cipherBuffer, algorithmType, mode, skBytesBuffer, ivBytesBuffer, plainTextBytesBuffer);

    // encrypt and decrypt test
    // std::cout << "### before dec, plain: "
    //           << std::string(plainTextBytesBuffer->data,
    //                  plainTextBytesBuffer->data + plainTextBytesBuffer->len)
    //           << std::endl;
    // std::cout << "#### decrypt, cipher len: " << cipherBuffer.len << std::endl;
    // bcos::bytes plain(cipherBuffer.len + symmetric_block_size(algorithmType));
    // OutputBuffer plainBuffer{plain.data(), plain.size()};
    // InputBuffer cipherBuffer2{cipherBuffer.data, cipherBuffer.len};
    // symmetric_decrypt(
    //     &plainBuffer, algorithmType, mode, skBytesBuffer, ivBytesBuffer, &cipherBuffer2);
    // std::cout << "#### error: " << get_last_error_msg() << std::endl;
    // std::cout << "#### decrypt end: " << std::string(plain.begin(), plain.end())
    //           << "# len: " << plainBuffer.len << ", size: " << plain.size() << std::endl;

    InputBuffer cipherBytes{cipherData.data(), cipherBuffer.len};
    // convert cipherBytes to hex
    auto hexCipher = *(bcos::toHexString(cipherData.data(), cipherData.data() + cipherBuffer.len));
    std::cout << "#### hexCipher: " << hexCipher << std::endl;
    fakedArgs.second->args[0] = (char*)hexCipher.data();
    fakedArgs.second->lengths[0] = hexCipher.size();

    auto hexIv =
        *(bcos::toHexString(ivBytesBuffer->data, ivBytesBuffer->data + ivBytesBuffer->len));
    // InputBuffer ivHexBuffer{(const unsigned char*)hexIv.data(), hexIv.size()};
    fakedArgs.second->args[3] = (char*)hexIv.data();
    fakedArgs.second->lengths[3] = hexIv.size();
    std::cout << "#### fakeMySQLParameters iv:" << fakedArgs.second->args[3] << std::endl;
    std::cout << "#### fakeMySQLParameters iv len:" << fakedArgs.second->lengths[3] << std::endl;

    auto hexSk =
        *(bcos::toHexString(skBytesBuffer->data, skBytesBuffer->data + skBytesBuffer->len));
    // InputBuffer skHexBuffer{(const unsigned char*)hexSk.data(), hexSk.size()};
    fakedArgs.second->args[4] = (char*)hexSk.data();
    fakedArgs.second->lengths[4] = hexSk.size();
    std::cout << "fakeMySQLParameters sk:" << fakedArgs.second->args[4] << std::endl;
    std::cout << "fakeMySQLParameters sk len:" << fakedArgs.second->lengths[4] << std::endl;

    std::cout << "args[0]" << fakedArgs.second->args[0] << std::endl;
    std::cout << "lengths[0]" << fakedArgs.second->lengths[0] << std::endl;
    std::cout << "args[1]" << fakedArgs.second->args[1] << std::endl;
    std::cout << "lengths[1]" << fakedArgs.second->lengths[1] << std::endl;
    std::cout << "args[2]" << fakedArgs.second->args[2] << std::endl;
    std::cout << "lengths[2]" << fakedArgs.second->lengths[2] << std::endl;
    std::cout << "args[3]" << fakedArgs.second->args[3] << std::endl;
    std::cout << "lengths[3]" << fakedArgs.second->lengths[3] << std::endl;
    std::cout << "args[4]" << fakedArgs.second->args[4] << std::endl;
    std::cout << "lengths[4]" << fakedArgs.second->lengths[4] << std::endl;

    std::cout << "#### before init" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    auto ret = sym_dec_init(fakedArgs.first, fakedArgs.second, message);
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
    auto decryptV = sym_dec(fakedArgs.first, fakedArgs.second, result, &length, nullptr, error);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // Print the execution time
    std::cout << " sym execution time: " << duration << " microseconds" << std::endl;
    auto plainText = plainTextBytesBuffer->data;
    std::cout << "#### sym pV: " << plainText << std::endl;
    std::cout << "#### sym pV len: " << plainTextBytesBuffer->len << std::endl;
    std::cout << "#### sym dV: " << decryptV << std::endl;
    std::cout << "#### sym dV len : " << strlen(decryptV) << std::endl;
    BOOST_CHECK(*plainText == *decryptV);
    sym_dec_deinit(fakedArgs.first);
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

void testSymDecUDFImpl(int algorithmType, int mode)
{
    srand(bcos::utcSteadyTime());
    // std::string plainText = "Hello, World, Hello Tomorrow.";
    std::string plainText = "caryliao";
    bcos::bytes plainTextBytes(plainText.begin(), plainText.end());
    // auto hexPlain = *(bcos::toHexString(plainTextBytes));
    // InputBuffer plainTextBytesBuffer{(const unsigned char*)hexPlain.data(), hexPlain.size()};
    InputBuffer plainTextBytesBuffer{
        (const unsigned char*)plainTextBytes.data(), plainTextBytes.size()};
    std::cout << "plain:" << plainTextBytesBuffer.data << std::endl;

    std::string iv = "0123456789abcdef";
    bcos::bytes ivBytes(iv.begin(), iv.end());
    InputBuffer ivBytesBuffer{(const unsigned char*)ivBytes.data(), ivBytes.size()};
    std::cout << "#### symmetric_key_bytes(algorithmType, mode):"
              << symmetric_key_bytes(algorithmType, mode) << std::endl;
    if (get_last_error() != 0)
    {
        std::cout << "error: " << get_last_error_msg() << std::endl;
    }
    bcos::bytes skBytes(symmetric_key_bytes(algorithmType, mode), 0);
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    symmetric_generate_key(&skBuffer, algorithmType, mode);

    // convert to hexedSk
    InputBuffer skBytesBuffer{(const unsigned char*)skBytes.data(), skBytes.size()};
    testSymDec(&plainTextBytesBuffer, algorithmType, mode, &ivBytesBuffer, &skBytesBuffer);
}

BOOST_AUTO_TEST_CASE(testSymDecUDF)
{
    std::cout << "run AES_128 ECB" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::AES_128), static_cast<int>(SymCrypto::OperationMode::ECB));
    std::cout << "run AES_128 CBC" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::AES_128), static_cast<int>(SymCrypto::OperationMode::CBC));
    std::cout << "run AES_128 CFB" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::AES_128), static_cast<int>(SymCrypto::OperationMode::CFB));
    std::cout << "run AES_128 CTR" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::AES_128), static_cast<int>(SymCrypto::OperationMode::CTR));
    std::cout << "run AES_192 OFB" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::AES_192), static_cast<int>(SymCrypto::OperationMode::OFB));
    std::cout << "run AES_256 CTR" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::AES_256), static_cast<int>(SymCrypto::OperationMode::CTR));

    std::cout << "run TrippleDES ECB" << std::endl;
    testSymDecUDFImpl(static_cast<int>(AlgorithmType::TrippleDES),
        static_cast<int>(SymCrypto::OperationMode::ECB));
    std::cout << "run TrippleDES CBC" << std::endl;
    testSymDecUDFImpl(static_cast<int>(AlgorithmType::TrippleDES),
        static_cast<int>(SymCrypto::OperationMode::CBC));
    std::cout << "run TrippleDES CFB" << std::endl;
    testSymDecUDFImpl(static_cast<int>(AlgorithmType::TrippleDES),
        static_cast<int>(SymCrypto::OperationMode::CFB));
    std::cout << "run TrippleDES OFB" << std::endl;
    testSymDecUDFImpl(static_cast<int>(AlgorithmType::TrippleDES),
        static_cast<int>(SymCrypto::OperationMode::OFB));

    std::cout << "run SM4 ECB" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::SM4), static_cast<int>(SymCrypto::OperationMode::ECB));
    std::cout << "run SM4 CBC" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::SM4), static_cast<int>(SymCrypto::OperationMode::CBC));
    std::cout << "run SM4 CFB" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::SM4), static_cast<int>(SymCrypto::OperationMode::CFB));
    std::cout << "run SM4 OFB" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::SM4), static_cast<int>(SymCrypto::OperationMode::OFB));
    std::cout << "run SM4 CTR" << std::endl;
    testSymDecUDFImpl(
        static_cast<int>(AlgorithmType::SM4), static_cast<int>(SymCrypto::OperationMode::CTR));
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
