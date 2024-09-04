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
 * @file paillier_dec.cpp
 * @author: caryliao
 * @date 2023-10-24
 */
#include "paillier_dec.h"
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

struct PaillierDecData
{
    void* keyPair = NULL;
};

// eg:paillier_dec(columnName, hexPk, hexSk, scale)
my_bool paillier_dec_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if (args->arg_count != 4)
    {
        const char* errorMsg =
            "\nUsage:paillier_dec(columnName, hexPk, hexSk, scale)"
            "\nExample:select paillier_dec(encrypted_salary,'0000080001...', '0000821BA...', 2)";
        snprintf(message, max_msg_size, "%s", errorMsg);
        return 1;
    }
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT ||
        args->arg_type[2] != STRING_RESULT || args->arg_type[3] != INT_RESULT)
    {
        snprintf(message, max_msg_size, "%s",
            "paillier_dec() requires as (string, string, string, int) as parameter types");
        return 1;
    }
    uint64_t pkBytesLen = args->lengths[1] / 2;
    uint64_t skBytesLen = args->lengths[2] / 2;
    unsigned char* pkBytesData = (unsigned char*)malloc(pkBytesLen);
    unsigned char* skBytesData = (unsigned char*)malloc(skBytesLen);
    if (!pkBytesData)
    {
        snprintf(message, max_msg_size, "%s", "allocate memory for public-key failed");
        return 1;
    }
    if (!skBytesData)
    {
        snprintf(message, max_msg_size, "%s", "allocate memory for private-key failed");
        return 1;
    }
    OutputBuffer pkBytesBuffer = {pkBytesData, pkBytesLen};
    OutputBuffer skBytesBuffer = {skBytesData, skBytesLen};
    InputBuffer pkHexBuffer = {(const unsigned char*)args->args[1], args->lengths[1]};
    InputBuffer skHexBuffer = {(const unsigned char*)args->args[2], args->lengths[2]};
    from_hex(&pkBytesBuffer, &pkHexBuffer);
    if (get_last_error() != 0)
    {
        snprintf(
            message, max_msg_size, "%s%s", "Invalid public key! error: ", get_last_error_msg());
        return 1;
    }
    from_hex(&skBytesBuffer, &skHexBuffer);
    if (get_last_error() != 0)
    {
        snprintf(
            message, max_msg_size, "%s%s", "Invalid private key! error: ", get_last_error_msg());
        return 1;
    }
    PaillierDecData* paillierDecDataPtr = new PaillierDecData;
    initid->ptr = (char*)paillierDecDataPtr;
    // load the public key
    InputBuffer pkBuffer = {pkBytesBuffer.data, pkBytesBuffer.len};
    // load the private key
    InputBuffer skBuffer = {skBytesBuffer.data, skBytesBuffer.len};
    paillierDecDataPtr->keyPair = paillier_load_keypair(&skBuffer, &pkBuffer);
    free(pkBytesData);
    free(skBytesData);
    if (get_last_error() != 0)
    {
        snprintf(message, max_msg_size, "%s%s", "Invalid key pair! error: ", get_last_error_msg());
        return 1;
    }
    return 0;
}

void paillier_dec_deinit(UDF_INIT* initid)
{
    if (initid->ptr)
    {
        PaillierDecData* paillierDecDataPtr = (PaillierDecData*)initid->ptr;
        if (paillierDecDataPtr->keyPair)
        {
            paillier_free_key_pair(paillierDecDataPtr->keyPair);
            paillierDecDataPtr->keyPair = NULL;
        }
    }
    free(initid->ptr);
    initid->ptr = NULL;
}

double paillier_dec(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length,
    char* is_null, char* error)
{
    // convert the args0 to bytes
    InputBuffer hexArgs = {(const unsigned char*)args->args[0], args->lengths[0]};
    OutputBuffer bytesArgsBuffer = {(unsigned char*)args->args[0], args->lengths[0] / 2};
    from_hex(&bytesArgsBuffer, &hexArgs);
    if (get_last_error() != 0)
    {
        snprintf(error, max_msg_size, "%s%s", "Invalid hex cipher! error: ", get_last_error_msg());
        return 1;
    }
    // paillier_decrypt
    InputBuffer ciphertext = {(const unsigned char*)bytesArgsBuffer.data, bytesArgsBuffer.len};
    BIGNUM* decrypted_significant = BN_new();
    int16_t exponent = 0;
    PaillierDecData* paillierDecDataPtr = (PaillierDecData*)initid->ptr;
    floating_point_paillier_decrypt(
        decrypted_significant, &exponent, &ciphertext, paillierDecDataPtr->keyPair);
    if (get_last_error() != 0)
    {
        snprintf(error, max_msg_size, "%s%s",
            "decrypt paillier cipher failed! error: ", get_last_error_msg());
        return 0;
    }
    int scale = *(int*)(args->args[3]);
    BigNum v;
    v.swap(decrypted_significant);
    FloatingPointNumber floatingPointNumber(std::move(v), exponent);
    auto floatingPointCodec = std::make_shared<FloatingPointCodec>();
    float50 decryptResult = floatingPointCodec->toFloat50(floatingPointNumber);
    double multiplier = pow(10.0, scale);
    double finalResult = round((double)decryptResult * multiplier) / multiplier;

    return finalResult;
}
