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
 * @file fast_ore.h
 * @author: yujiechen
 * @date 2023-08-25
 */
#ifndef __FAST_ORE_H__
#define __FAST_ORE_H__

#include "ppc-framework/libwrapper/Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

int fast_ore_key_bytes();

uint64_t fast_ore_get_cipher_size(uint64_t plainSize, bool hex);
uint64_t fast_ore_get_float_cipher_size(uint64_t plainSize, bool hex);
uint64_t fast_ore_get_plain_size(uint64_t cipherSize, bool hex);

/**
 * @brief generate key for the fastORE
 *
 * @param key the generated key
 */
void fast_ore_generate_key(OutputBuffer* key);

/**
 * @brief encrypt the plain data into cipher data using given key
 *
 * @param cipher the encrypted data
 * @param key the key used to encrypt
 * @param plain the plain-data
 */
void string_fast_ore_encrypt(
    OutputBuffer* cipher, InputBuffer const* key, InputBuffer const* plain, bool hexEncode);

/**
 * @brief decrypt the cipher data into plain data using given key
 *
 * @param plain the decrypted plain data
 * @param key the key used to decrypt
 * @param cipher the cipher
 */
void string_fast_ore_decrypt(
    OutputBuffer* plain, InputBuffer const* key, InputBuffer const* cipher, bool hexEncode);

/**
 * @brief encrypt the plain data into cipher data using given key
 *
 * @param cipher the encrypted data
 * @param key the key used to encrypt
 * @param plain the plain-data
 */
void integer_fast_ore_encrypt(
    OutputBuffer* cipher, InputBuffer const* key, int64_t const& plain, bool hexEncode);

/**
 * @brief decrypt the cipher data into plain data using given key
 *
 * @param plain the decrypted plain data
 * @param key the key used to decrypt
 * @param cipher the cipher
 */
void integer_fast_ore_decrypt(
    int64_t* plain, InputBuffer const* key, InputBuffer const* cipher, bool hexEncode);

/**
 * @brief encrypt the plain data into cipher data using given key
 *
 * @param cipher the encrypted data
 * @param key the key used to encrypt
 * @param plain the plain-data
 */
void float_fast_ore_encrypt(
    OutputBuffer* cipher, InputBuffer const* key, InputBuffer const* plain, bool hexEncode);

/**
 * @brief decrypt the cipher data into plain data using given key
 *
 * @param plain the decrypted plain data
 * @param key the key used to decrypt
 * @param cipher the cipher
 */
void float_fast_ore_decrypt(
    OutputBuffer* plain, InputBuffer const* key, InputBuffer const* cipher, bool hexEncode);

/**
 * @brief compare between cipher1 and cipher2
 *
 * @param cipher1
 * @param cipher2
 * @return int
 */
int fast_ore_compare(InputBuffer const* cipher1, InputBuffer const* cipher2);
#ifdef __cplusplus
}
#endif
#endif