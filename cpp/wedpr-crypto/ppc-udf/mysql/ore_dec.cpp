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
 * @file ore_dec.cpp
 * @author: caryliao
 * @date 2023-10-26
 */
#include "ore_dec.h"
#include "ppc-crypto-c-sdk/fast_ore.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-crypto-c-sdk/utils/utilities.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

bool isHexInDec = true;
struct OreDecData {
    InputBuffer oreSk;
    char* oreDecryptResult = NULL;
};

// eg:ore_dec(columnName, sk)
my_bool ore_dec_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if (args->arg_count != 2)
    {
        const char* errorMsg =
            "\nUsage:ore_dec(columnName, hexSk)"
            "\nExample:select ore_dec(encrypted_name, '303132...') as user_name from t_user";
        snprintf(message, max_msg_size, "%s", errorMsg);
        return 1;
    }

    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT)
    {
        snprintf(message, max_msg_size, "%s", "ore_dec() requires (string, string) as parameter types");
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
    OreDecData* oreDataPtr = new OreDecData;
    initid->ptr = (char*)oreDataPtr;
    oreDataPtr->oreSk.data = skBytesBuffer.data;
    oreDataPtr->oreSk.len = skBytesBuffer.len;
    oreDataPtr->oreDecryptResult = (char*)malloc(fast_ore_get_plain_size(args->lengths[0] / 2, isHexInDec));
    if (!oreDataPtr->oreDecryptResult)
    {
        snprintf(message, max_msg_size, "%s", "malloc memory for oreDecryptResult failed!");
        return 1;
    }

    return 0;
}

void ore_dec_deinit(UDF_INIT* initid)
{
    if (initid->ptr)
    {
        OreDecData* oreDataPtr = (OreDecData*)initid->ptr;
        if (oreDataPtr->oreSk.data)
        {
            free((void*)oreDataPtr->oreSk.data);
            oreDataPtr->oreSk.data = NULL;
        }
        if (oreDataPtr->oreDecryptResult)
        {
            free(oreDataPtr->oreDecryptResult);
            oreDataPtr->oreDecryptResult = NULL;
        }
        free(initid->ptr);
        initid->ptr = NULL;
    }
}

char* ore_dec(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null,
    char* error)
{
    InputBuffer ciphertext = {(const unsigned char*)args->args[0], args->lengths[0]};
    OreDecData* oreDataPtr = (OreDecData*)initid->ptr;
    // ore_decrypt
    OutputBuffer plainBuffer = {
        (unsigned char*)oreDataPtr->oreDecryptResult, fast_ore_get_plain_size(ciphertext.len, isHexInDec)};
    string_fast_ore_decrypt(&plainBuffer, &oreDataPtr->oreSk, &ciphertext, isHexInDec);
    if (get_last_error() != 0)
    {
        snprintf(error, max_msg_size, "%s", "decrypt ore cipher failed");
        return NULL;
    };
    *length = plainBuffer.len;
    return oreDataPtr->oreDecryptResult;
}
