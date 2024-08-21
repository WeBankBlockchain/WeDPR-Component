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
 * @file floating_point_paillier.h
 * @author: yujiechen
 * @date 2023-08-20
 */
#ifndef __FLOATING_POINT_PAILLIER_H__
#define __FLOATING_POINT_PAILLIER_H__
#include "openssl/bn.h"
#include "ppc-framework/libwrapper/Buffer.h"
#ifdef __cplusplus
extern "C" {
#endif

unsigned int floating_point_paillier_cipher_bytes(unsigned int keyBits);
/**
 * @brief encrypt {value*10^exponent} using paillier crt-optimization
 *
 * @param cipherBytes the encrypted result
 * @param value the value to be encrypted
 * @param exponent the exponent of the value to be encrypted
 * @param keypair the keypair used to encrypt
 */
void floating_point_paillier_encrypt_fast(
    OutputBuffer* cipherBytes, BIGNUM const* value, int16_t exponent, void* keypair);

void floating_point_paillier_encrypt_fast_without_precompute(OutputBuffer* cipherBytes,
    BIGNUM const* value, int16_t exponent, InputBuffer const* sk, InputBuffer const* pk);

/**
 * @brief encrypt {value*10^exponent} withou using paillier crt-optimization
 *
 * @param cipherBytes the encrypted result
 * @param value the value to be encrypted
 * @param exponent the exponent of the value to be encrypted
 * @param public_key the public_key used to encrypt
 */
void floating_point_paillier_encrypt(
    OutputBuffer* cipherBytes, BIGNUM const* value, int16_t _exponent, void* public_key);
void floating_point_paillier_encrypt_without_precompute(
    OutputBuffer* cipherBytes, BIGNUM const* value, int16_t _exponent, InputBuffer const* pkBytes);

/**
 * @brief paillier decrypt
 *
 * @param value the decrypted value
 * @param exponent the exponent of the decrypted value
 * @param cipher the cipher to be decrypted
 * @param keypair the keyPair used to decrypt
 */
void floating_point_paillier_decrypt(
    BIGNUM* value, int16_t* exponent, InputBuffer const* cipher, void* keypair);
void floating_point_paillier_decrypt_without_precompute(BIGNUM* value, int16_t* exponent,
    InputBuffer const* cipher, InputBuffer const* sk, InputBuffer const* pk);

/**
 * @brief paillier add
 *
 * @param cipherBytes the paillier cipher add result
 * @param cipher1
 * @param cipher2
 * @param public_key
 */
void floating_point_paillier_add(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, void* public_key);
void floating_point_paillier_add_without_precompute(OutputBuffer* cipherBytes,
    InputBuffer const* cipher1, InputBuffer const* cipher2, InputBuffer const* pkBytes);

/**
 * @brief paillier sub
 *
 * @param cipherBytes the paillier cipher sub result
 * @param cipher1
 * @param cipher2
 * @param public_key
 */
void floating_point_paillier_sub(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, void* public_key);
void floating_point_paillier_sub_without_precompute(OutputBuffer* cipherBytes,
    InputBuffer const* cipher1, InputBuffer const* cipher2, InputBuffer const* pkBytes);

/**
 * @brief paillier-scalar-multipy
 *
 * @param cipherBytes the paillier cipher multipy result
 * @param v the value
 * @param exponent the exponent of the value
 * @param cipher the cipher
 * @param public_key
 */
void floating_point_paillier_scalaMul(OutputBuffer* cipherBytes, BIGNUM const* v, int16_t exponent,
    InputBuffer const* cipher, void* public_key);
void floating_point_paillier_scalaMul_without_precompute(OutputBuffer* cipherBytes, BIGNUM const* v,
    int16_t exponent, InputBuffer const* cipher, InputBuffer const* pkBytes);
#ifdef __cplusplus
}
#endif
#endif