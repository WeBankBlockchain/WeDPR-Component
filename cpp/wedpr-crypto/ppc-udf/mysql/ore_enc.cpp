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
 * @file ore_enc.cpp
 * @author: caryliao
 * @date 2023-11-30
 */
#include "ore_enc.h"
#include "ppc-crypto-c-sdk/fast_ore.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-crypto-c-sdk/utils/utilities.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

bool isHexInEnc = true;
struct OreEncData {
    InputBuffer oreSk;
    char* oreEncryptResult = NULL;
};

// eg:ore_enc(columnName, sk)
my_bool ore_enc_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if (args->arg_count != 2)
    {
        const char* errorMsg =
            "\nUsage:ore_enc(columnName, hexSk)"
            "\nExample:select ore_enc(name, '303132...') as user_name from t_user";
        snprintf(message, max_msg_size, "%s", errorMsg);
        return 1;
    }

    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT)
    {
        snprintf(message, max_msg_size, "%s", "ore_enc() requires (string, string) as parameter types");
        return 1;
    }
    uint64_t skBytesLen = args->lengths[1] / 2;
    unsigned char* skBytesData = (unsigned char*)malloc(skBytesLen);
    if (!skBytesData)
    {
        snprintf(message, max_msg_size, "%s", "allocate memory for sk failed");
        return 1;
    }
    OutputBuffer skBytesBuffer = {skBytesData, skBytesLen};
    InputBuffer skHexBuffer = {(const unsigned char*)args->args[1], args->lengths[1]};
    from_hex(&skBytesBuffer, &skHexBuffer);
    if (get_last_error() != 0)
    {
        snprintf(message, max_msg_size, "%s", "invalid hexSk");
        return 1;
    }
    OreEncData* oreDataPtr = new OreEncData;
    initid->ptr = (char*)oreDataPtr;
    oreDataPtr->oreSk.data = skBytesBuffer.data;
    oreDataPtr->oreSk.len = skBytesBuffer.len;
    oreDataPtr->oreEncryptResult = (char*)malloc(fast_ore_get_cipher_size(args->lengths[0], isHexInEnc));
    if (!oreDataPtr->oreEncryptResult)
    {
        snprintf(message, max_msg_size, "%s", "malloc memory for oreEncryptResult failed!");
        return 1;
    }

    return 0;
}

void ore_enc_deinit(UDF_INIT* initid)
{
    if (initid->ptr)
    {
        OreEncData* oreDataPtr = (OreEncData*)initid->ptr;
        if (oreDataPtr->oreSk.data)
        {
            free((void*)oreDataPtr->oreSk.data);
            oreDataPtr->oreSk.data = NULL;
        }
        if (oreDataPtr->oreEncryptResult)
        {
            free(oreDataPtr->oreEncryptResult);
            oreDataPtr->oreEncryptResult = NULL;
        }
        free(initid->ptr);
        initid->ptr = NULL;
    }
}

char* ore_enc(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null,
    char* error)
{
    InputBuffer plaintext = {(const unsigned char*)args->args[0], args->lengths[0]};
    OreEncData* oreDataPtr = (OreEncData*)initid->ptr;
    // ore_encrypt
    OutputBuffer cipherBuffer = {
        (unsigned char*)oreDataPtr->oreEncryptResult, fast_ore_get_cipher_size(plaintext.len, isHexInEnc)};
    string_fast_ore_encrypt(&cipherBuffer, &oreDataPtr->oreSk, &plaintext, isHexInEnc);
    if (get_last_error() != 0)
    {
        snprintf(error, max_msg_size, "%s", "encrypt ore cipher failed");
        return NULL;
    };
    *length = cipherBuffer.len;
    return oreDataPtr->oreEncryptResult;
}
