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
 * @file homo_ihc.h
 * @author: asherli
 * @date 2023-11-28
 */

#ifndef __HOMO_IHC_H__
#define __HOMO_IHC_H__
#include "openssl/bn.h"
#include "ppc-framework/libwrapper/Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void ihc_generate_key(OutputBuffer* key, int mode);
void ihc_encrypt(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* _key, BIGNUM const* _plain);
void ihc_decrypt(BIGNUM* _result, int _mode, InputBuffer const* _key, InputBuffer const* cipher);

void ihc_add(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2);
void ihc_sub(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2);
void ihc_scalaMul(OutputBuffer* cipherBytes, int mode, BIGNUM const* v, InputBuffer const* cipher);

unsigned int ihc_key_bytes(int mode);
uint64_t ihc_cipher_bytes(int mode);
#ifdef __cplusplus
}
#endif
#endif