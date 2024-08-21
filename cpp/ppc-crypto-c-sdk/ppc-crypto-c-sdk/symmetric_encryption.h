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
 * @file symmetric_encryption.h
 * @author: yujiechen
 * @date 2023-08-25
 */
#ifndef __SYMMETRIC_ENCRYPTION_H__
#define __SYMMETRIC_ENCRYPTION_H__

#include "ppc-framework/libwrapper/Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif
enum AlgorithmType
{
    AES_128,
    AES_192,
    AES_256,
    TrippleDES,
    SM4
};
unsigned int symmetric_block_size(int algorithm);
int symmetric_key_bytes(int algorithm, int mode);
/**
 * @brief generate key for symmetric encryption
 *
 * @param _sk the generated key
 */
void symmetric_generate_key(OutputBuffer* _sk, int algorithm, int mode);

/**
 * @brief encrypt the plainData into cipher using given algorithm and mode
 *
 * @param _cipher the encrypted cipher
 * @param algorithm the symmetric encryption algorithm, e.g. AES/DES/SM4
 * @param mode the encryption mode
 * @param sk the sk used to encrypt
 * @param plainData the plainData
 */
void symmetric_encrypt(OutputBuffer* cipher, int algorithm, int mode, InputBuffer const* sk,
    InputBuffer const* iv, InputBuffer const* plainData);

/**
 * @brief decrypt the cipher
 *
 * @param algorithm
 * @param mode
 * @param sk
 * @param cipher
 */

/**
 * @brief decrypt the given cipher into plain using given sk/algorithm and mode
 *
 * @param plain the decrypted plainData
 * @param algorithm the symmetric encryption algorithm, e.g. AES/DES/SM4
 * @param mode the encryption mode
 * @param sk the sk used to decrypt
 * @param cipher the cipher
 */
void symmetric_decrypt(OutputBuffer* plain, int algorithm, int mode, InputBuffer const* sk,
    InputBuffer const* iv, InputBuffer const* cipher);

#ifdef __cplusplus
}
#endif
#endif