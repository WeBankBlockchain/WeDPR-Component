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
 * @file paillier_sum.cpp
 * @author: yujiechen
 * @date 2023-08-18
 */
#include "paillier_sum.h"
#include "ppc-crypto-c-sdk/floating_point_paillier.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-crypto-c-sdk/utils/utilities.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

struct SumResult
{
    uint64_t max_cipher_bytes = 0;
    void* public_key = NULL;
    char* sum_result = NULL;
    int64_t add_count = 0;
};

my_bool paillier_sum_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if (args->arg_count != 2)
    {
        const char* errorMsg =
            "invalid argument: paillier_sum requires the columnName and the publicKey, "
            "e.g. paillier_sum(id, publicKey)";
        snprintf(message, max_msg_size, "%s", errorMsg);
        return 1;
    }
    /// TODO: check the parameters
    // convert the hexed public key to bytes
    // TODO: decrease the overhead
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
    SumResult* resultBuffer = new SumResult;
    // load the public key
    InputBuffer pkBuffer = {pkBytesBuffer.data, pkBytesBuffer.len};
    resultBuffer->public_key = (void*)paillier_load_public_key(&pkBuffer);
    // release the allocated memory
    free(pkBytesData);

    if (!resultBuffer->public_key)
    {
        snprintf(message, max_msg_size, "%s%s", "Invalid public key:", get_last_error_msg());
        return 1;
    }

    // allocate memory for the result
    resultBuffer->max_cipher_bytes = floating_point_paillier_cipher_bytes(
        paillier_key_bits_from_public_key(resultBuffer->public_key));
    initid->maybe_null = 1;
    // will convert the result to hex when calling sum
    initid->max_length = 2 * resultBuffer->max_cipher_bytes;
    // Note: The MySQL UDF engine allocates 255 bytes as the buffer for a string return value. If
    // the result fits within this limit, simply place the result into this buffer and return the
    // pointer. If the result is bigger, you must use a string buffer that you have allocated.
    // refer to: https://docstore.mik.ua/orelly/weblinux2/mysql/ch14_01.htm
    resultBuffer->sum_result = (char*)malloc(initid->max_length);
    if (!resultBuffer->sum_result)
    {
        snprintf(message, max_msg_size, "%s", "Malloc memory for sum_result failed!");
        return 1;
    }
    initid->ptr = (char*)resultBuffer;
    if (!initid->ptr)
    {
        snprintf(message, max_msg_size, "%s", "Malloc memory for initid->ptr failed!");
    }
    return 0;
}

void paillier_sum_deinit(UDF_INIT* initid)
{
    paillier_sum_clear(initid, NULL, NULL);
    if (initid->ptr)
    {
        delete initid->ptr;
    }
    initid->ptr = NULL;
}

void paillier_sum_clear(UDF_INIT* initid, char* is_null, char* error)
{
    SumResult* result = (SumResult*)(initid->ptr);
    result->add_count = 0;
    result->max_cipher_bytes = 0;
    if (result->public_key)
    {
        paillier_free_key_pair(result->public_key);
        result->public_key = NULL;
    }
    if (result->sum_result)
    {
        free(result->sum_result);
        result->sum_result = NULL;
    }
}

void paillier_sum_reset(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* error)
{
    paillier_sum_clear(initid, is_null, error);
    paillier_sum_add(initid, args, is_null, error);
}

void paillier_sum_add(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* error)
{
    if (args->args[0] == NULL)
    {
        return;
    }
    SumResult* resultBuffer = (SumResult*)initid->ptr;
    // convert the args0 to bytes
    InputBuffer hexArgs = {(const unsigned char*)args->args[0], args->lengths[0]};
    OutputBuffer bytesArgsBuffer = {(unsigned char*)args->args[0], args->lengths[0] / 2};
    from_hex(&bytesArgsBuffer, &hexArgs);
    if (get_last_error() != 0)
    {
        snprintf(error, max_msg_size, "%s", "Invalid hex cipher!");
        return;
    }
    // the first add
    if (0 == resultBuffer->add_count)
    {
        memcpy((void*)resultBuffer->sum_result, (const void*)bytesArgsBuffer.data,
            bytesArgsBuffer.len);
        resultBuffer->add_count++;
        return;
    }
    // paillier_add
    InputBuffer c1 = {
        (const unsigned char*)resultBuffer->sum_result, resultBuffer->max_cipher_bytes};
    InputBuffer c2 = {(const unsigned char*)bytesArgsBuffer.data, bytesArgsBuffer.len};
    OutputBuffer result = {
        (unsigned char*)resultBuffer->sum_result, resultBuffer->max_cipher_bytes};
    floating_point_paillier_add(&result, &c1, &c2, resultBuffer->public_key);
    resultBuffer->add_count++;
}

char* paillier_sum(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length,
    char* is_null, char* error)
{
    SumResult* sum_result = (SumResult*)initid->ptr;
    // convert to hex
    OutputBuffer resultBuffer = {(unsigned char*)sum_result->sum_result, initid->max_length};
    InputBuffer bytesBuffer = {
        (unsigned char*)sum_result->sum_result, sum_result->max_cipher_bytes};
    to_hex(&resultBuffer, &bytesBuffer);
    *length = resultBuffer.len;
    return sum_result->sum_result;
}
