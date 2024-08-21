/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file Common.cpp
 * @author: yujiechen
 * @date 2023-08-30
 */
#include "Common.h"
#include "JNIException.h"
#include "openssl/err.h"
#include <bcos-utilities/Common.h>

// convert java bytes to InputBuffer
InputBuffer convertToInputBuffer(bcos::bytes& resultBuffer, JNIEnv* env, jbyteArray arrayData)
{
    resultBuffer.resize(env->GetArrayLength(arrayData));
    env->GetByteArrayRegion(arrayData, 0, resultBuffer.size(), (jbyte*)resultBuffer.data());
    return InputBuffer{resultBuffer.data(), resultBuffer.size()};
}

jbyteArray BigNumToJavaBigIntegerBytes(JNIEnv* env, BIGNUM* value)
{
    if (!value)
    {
        THROW_JNI_EXCEPTION(env, "Not support convert null big-num to big-integer!");
        return NULL;
    }
    auto resultLen = BN_num_bytes(value) + 1;
    bcos::bytes valueBin(resultLen);
    if (BN_bn2bin(value, valueBin.data() + 1) < 0)
    {
        THROW_JNI_EXCEPTION(
            env, "BN_bn2bin error: " + std::string(ERR_error_string(ERR_get_error(), NULL)));
        return NULL;
    }
    // negative case
    if (BN_is_negative(value))
    {
        bool carry = true;
        for (int64_t i = resultLen - 1; i >= 0; i--)
        {
            valueBin[i] ^= 0xff;
            if (carry)
            {
                carry = (++(valueBin[i])) == 0;
            }
        }
        valueBin[0] |= 0x80;
    }
    else
    {
        valueBin[0] = 0x00;
    }
    jbyteArray ret = env->NewByteArray(resultLen);
    env->SetByteArrayRegion(ret, 0, resultLen, (jbyte*)valueBin.data());
    return ret;
}

// convert java big-integer to openssl BIGNUM
ppc::crypto::BigNum JavaBigIntegerToBigNum(JNIEnv* env, jbyteArray bigIntegerData)
{
    bcos::bytes javaBigIntegerBytes;
    convertToInputBuffer(javaBigIntegerBytes, env, bigIntegerData);
    unsigned char* data = javaBigIntegerBytes.data();
    unsigned char* pData = data;
    auto len = javaBigIntegerBytes.size();
    ppc::crypto::BigNum result;
    // zero
    if (0 == len)
    {
        BN_zero(result.bn().get());
        return result;
    }
    bool negative = (data[0] & 0x80) != 0;
    // convert to two's complement if negative
    bcos::bytes twosComplement(len, 0);
    if (negative)
    {
        bool carry = true;
        for (int i = len - 1; i >= 0; i--)
        {
            twosComplement[i] = (data[i] ^ 0xFF);
            if (carry)
            {
                carry = (++twosComplement[i]) == 0;
            }
        }
        pData = twosComplement.data();
    }
    // convert to BIGNUM
    if (!BN_bin2bn(pData, len, result.bn().get()))
    {
        BN_clear_free(result.bn().get());
        THROW_JNI_EXCEPTION(env, "convert java bigInteger data to BigNum error" +
                                     std::string(ERR_error_string(ERR_get_error(), NULL)));
    }
    if (negative)
    {
        BN_set_negative(result.bn().get(), 1);
    }
    return result;
}