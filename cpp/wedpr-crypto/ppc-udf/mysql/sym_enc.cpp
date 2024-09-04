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
 * @file sym_enc.cpp
 * @author: caryliao
 * @date 2023-11-29
 */
#include "sym_enc.h"
#include "ppc-crypto-c-sdk/symmetric_encryption.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-crypto-c-sdk/utils/utilities.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;
using namespace bcos;

struct SymEncData {
    int algorithmType = 0;
    int mode = 0;
    InputBuffer iv;
    InputBuffer symSk;
    char* symEncryptResult = NULL;
};


// eg:sym_enc(columnName, algorithmType, mode, hexIv, hexSk)
my_bool sym_enc_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    SymEncData* data = new SymEncData;
    initid->ptr = (char*)data;
    
    if (args->arg_count != 5)
    {
        const char* errorMsg =
            "\nUsage:sym_enc(columnName, algorithmType, mode, hexIv, hexSk)"
            "\nalgorithmType(0:AES_128, 1:AES_192, 2:AES_256, 3:TrippleDES, 4:SM4), mode(0:ECB, "
            "1:CBC, 2:CFB, 3:OFB, 4:CTR)";
        snprintf(message, max_msg_size, "%s", errorMsg);
        return 1;
    }
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != INT_RESULT ||
        args->arg_type[2] != INT_RESULT || args->arg_type[3] != STRING_RESULT ||
        args->arg_type[4] != STRING_RESULT)
    {
        snprintf(message, max_msg_size, "%s", "sym_enc() requires (string, int, int, string, string) as parameters");
        return 1;
    }
    int algorithmType = *(int*)(args->args[1]);
    int mode = *(int*)(args->args[2]);
    // check algorithmType and mode in [0, 4]
    if (algorithmType < 0 || algorithmType > 4)
    {
        snprintf(message, max_msg_size, "%s", "invalid algorithmType, algorithmType must be 0, 1, 2, 3 or 4");
        return 1;
    }
    if (mode < 0 || mode > 4)
    {
        snprintf(message, max_msg_size, "%s", "invalid mode, mode must be 0, 1, 2, 3 or 4");
        return 1;
    }
    // algorithmType is 3 and mode is 4 is not match
    if (algorithmType == 3 && mode == 4)
    {
        snprintf(message, max_msg_size, "%s", "TrippleDES does not support CTR mode");
        return 1;
    }
    data->algorithmType = algorithmType;
    data->mode = mode;
    unsigned char* ivBytesData = (unsigned char*)malloc(args->lengths[3] / 2);
    unsigned char* skBytesData = (unsigned char*)malloc(args->lengths[4] / 2);
    if (!ivBytesData)
    {
        snprintf(message, max_msg_size, "%s", "allocate memory for iv failed");
        return 1;
    }
    if (!skBytesData)
    {
        snprintf(message, max_msg_size, "%s", "allocate memory for sk failed");
        return 1;
    }
    OutputBuffer ivBytesBuffer = {ivBytesData, args->lengths[3] / 2};
    OutputBuffer skBytesBuffer = {skBytesData, args->lengths[4] / 2};
    InputBuffer ivHexBuffer = {(const unsigned char*)args->args[3], args->lengths[3]};
    InputBuffer skHexBuffer = {(const unsigned char*)args->args[4], args->lengths[4]};

    from_hex(&ivBytesBuffer, &ivHexBuffer);
    if (get_last_error() != 0)
    {
        snprintf(message, max_msg_size, "%s", "Invalid hex iv!");
        return 1;
    }
    from_hex(&skBytesBuffer, &skHexBuffer);
    if (get_last_error() != 0)
    {
        snprintf(message, max_msg_size, "%s", "Invalid hex sk!");
        return 1;
    }
    data->iv.data = ivBytesBuffer.data;
    data->iv.len = ivBytesBuffer.len;

    data->symSk.data = skBytesBuffer.data;
    data->symSk.len = skBytesBuffer.len;

    data->symEncryptResult = (char*)malloc(symmetric_block_size(data->algorithmType) + args->lengths[0]);
    if (!data->symEncryptResult)
    {
        snprintf(message, max_msg_size, "%s", "Malloc memory for symEncryptResult failed!");
        return 1;
    }

    return 0;
}

void sym_enc_deinit(UDF_INIT* initid)
{
    if (initid->ptr)
    {
        SymEncData* data = (SymEncData*)initid->ptr;
        if(data->symEncryptResult)
        {
            free(data->symEncryptResult);
            data->symEncryptResult = NULL;
        }
        if(data->iv.data)
        {
            free((void*)data->iv.data);
            data->iv.data = NULL;
        }
        if(data->symSk.data)
        {
            free((void*)data->symSk.data);
            data->symSk.data = NULL;
        }
        free(initid->ptr);
        initid->ptr = NULL;
    }
}

char* sym_enc(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null,
    char* error)
{
    InputBuffer plainText = {(const unsigned char*)args->args[0], args->lengths[0]};
    SymEncData* data = (SymEncData*)initid->ptr;
    OutputBuffer cipherBuffer = {(unsigned char*)(data->symEncryptResult),
        symmetric_block_size(data->algorithmType) + args->lengths[0]};
    symmetric_encrypt(&cipherBuffer, data->algorithmType, data->mode, &data->symSk, &data->iv, &plainText);
    if (get_last_error() != 0)
    {
        const char* errorMsg = get_last_error_msg();
        snprintf(error, max_msg_size, "%s", errorMsg);
        return NULL;
    };
    *length = cipherBuffer.len;
    return data->symEncryptResult;
}
