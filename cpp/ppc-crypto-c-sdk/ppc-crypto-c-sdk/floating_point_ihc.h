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
 * @file floating_point_ihc.h
 * @author: asherli
 * @date 2023-11-28
 */

#ifndef __FLOATING_POINT_IHC_H__
#define __FLOATING_POINT_IHC_H__
#include "openssl/bn.h"
#include "ppc-framework/libwrapper/Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif
void ihc_floating_encrypt(OutputBuffer* cipherBytes, int mode, InputBuffer const* _key,
    BIGNUM const* value, int16_t _exponent);
void ihc_floating_decrypt(
    BIGNUM* value, int16_t* exponent, int mode, InputBuffer const* key, InputBuffer const* cipher);
void ihc_floating_add(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2);
void ihc_floating_sub(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2);
void ihc_floating_scalaMul(OutputBuffer* cipherBytes, int mode, BIGNUM const* v, int16_t exponent,
    InputBuffer const* cipher);

uint64_t ihc_floating_cipher_bytes(int mode);

#ifdef __cplusplus
}
#endif
#endif