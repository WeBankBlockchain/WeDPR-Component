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
 * @file paillier_enc.cpp
 * @author: caryliao
 * @date 2023-10-24
 */
#include "paillier_enc.h"
#include "ppc-crypto-c-sdk/floating_point_paillier.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-crypto-c-sdk/utils/utilities.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;
using namespace ppc;
using namespace ppc::homo;
using namespace ppc::crypto;

struct PaillierEncData
{
    void* publicKey = NULL;
    char* cipher_result;
};

// eg:paillier_enc(columnName, hexPk)
my_bool paillier_enc_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if (args->arg_count != 2)
    {
        const char* errorMsg =
            "\nUsage:paillier_enc(columnName, hexPk)"
            "\nExample:select paillier_enc(salary,'0000080001...')";
        snprintf(message, max_msg_size, "%s", errorMsg);
        return 1;
    }
    // TODO 使用double类型，目前没有获取到参数值，一直是0.00000
    // double plainValue = *(double*)args->args[0];
    double plainValue = strtod(args->args[0], NULL);
    // std::cout << "double plainValue:" <<  plainValue << std::endl;
    // snprintf(message, max_msg_size, "%f", plainValue);
    // return 1;
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT)
    {
        snprintf(message, max_msg_size, "%s",
            "paillier_enc() requires as (string, string) as parameter types");
        return 1;
    }
    uint64_t pkBytesLen = args->lengths[1] / 2;
    unsigned char* pkBytesData = (unsigned char*)malloc(pkBytesLen);
    if (!pkBytesData)
    {
        snprintf(message, max_msg_size, "%s", "allocate memory for public-key failed");
        return 1;
    }
    OutputBuffer pkBytesBuffer = {pkBytesData, pkBytesLen};
    InputBuffer pkHexBuffer = {(const unsigned char*)args->args[1], args->lengths[1]};
    from_hex(&pkBytesBuffer, &pkHexBuffer);
    if (get_last_error() != 0)
    {
        snprintf(
            message, max_msg_size, "%s%s", "Invalid public key! error: ", get_last_error_msg());
        return 1;
    }
    PaillierEncData* paillierEncDataPtr = new PaillierEncData;
    initid->ptr = (char*)paillierEncDataPtr;
    // load the public key
    InputBuffer pkBuffer = {pkBytesBuffer.data, pkBytesBuffer.len};
    paillierEncDataPtr->publicKey = paillier_load_public_key(&pkBuffer);
    free(pkBytesData);
    if (get_last_error() != 0)
    {
        snprintf(message, max_msg_size, "%s%s", "Invalid key pair! error: ", get_last_error_msg());
        return 1;
    }
    auto keyBits = paillier_key_bits_from_public_key(paillierEncDataPtr->publicKey);
    auto cipherBytes = floating_point_paillier_cipher_bytes(keyBits);
    initid->max_length = cipherBytes;
    paillierEncDataPtr->cipher_result = (char*)malloc(initid->max_length);
    if (!paillierEncDataPtr->cipher_result)
    {
        snprintf(message, max_msg_size, "%s", "Malloc memory for cipher_result failed!");
        return 1;
    }
    return 0;
}

void paillier_enc_deinit(UDF_INIT* initid)
{
    std::cout << "#### paillier_enc_deinit" << std::endl;
    if (initid->ptr)
    {
        PaillierEncData* paillierEncDataPtr = (PaillierEncData*)initid->ptr;
        if (paillierEncDataPtr->publicKey)
        {
            paillier_free_public_key(paillierEncDataPtr->publicKey);
            paillierEncDataPtr->publicKey = NULL;
        }
        if (paillierEncDataPtr->cipher_result)
        {
            free(paillierEncDataPtr->cipher_result);
            paillierEncDataPtr->cipher_result = NULL;
        }
        free(initid->ptr);
    }
    initid->ptr = NULL;
}

char* paillier_enc(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length,
    char* is_null, char* error)
{
    PaillierEncData* paillierEncDataPtr = (PaillierEncData*)initid->ptr;
    float50 plainValue(args->args[0]);
    auto floatingPointCodec = std::make_shared<FloatingPointCodec>();
    auto fpn = floatingPointCodec->toFloatingPoint(std::string(args->args[0]));
    int16_t exponent = fpn.exponent;
    OutputBuffer cipherBuffer{
        (unsigned char*)paillierEncDataPtr->cipher_result, initid->max_length};
    floating_point_paillier_encrypt(
        &cipherBuffer, fpn.value.bn().get(), exponent, paillierEncDataPtr->publicKey);
    if (get_last_error() != 0)
    {
        const char* errorMsg = get_last_error_msg();
        snprintf(error, max_msg_size, "%s%s", "encrypt failed! error: ", errorMsg);
        return NULL;
    }
    // OutputBuffer resultBuffer = {(unsigned char*)paillierEncDataPtr->cipher_result,
    // cipherBuffer.len * 2}; InputBuffer bytesBuffer = {(unsigned char*)cipherBuffer.data,
    // cipherBuffer.len}; to_hex(&resultBuffer, &bytesBuffer);
    *length = initid->max_length;
    return paillierEncDataPtr->cipher_result;
}
