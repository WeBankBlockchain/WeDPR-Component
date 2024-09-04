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
 * @file homo_paillier.h
 * @author: yujiechen
 * @date 2023-08-11
 */
#ifndef __HOMO_PAILLIER_H__
#define __HOMO_PAILLIER_H__

#include "openssl/bn.h"
#include "ppc-framework/libwrapper/Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get the paillier max public key bytes according to the keyBits
 *
 * @param key_bits e.g. 512/1024/2048/3096
 * @return unsigned int the public key bytes
 */
unsigned int paillier_max_public_key_bytes(int key_bits);

/**
 * @brief get the paillier random r bytes according to the keyBits
 * @param key_bits e.g. 512/1024/2048/3096
 */
unsigned int paillier_r_bytes_len(int key_bits);

/**
 * @brief get the paillier max private key bytes according to the keyBits
 *
 * @param key_bits e.g. 512/1024/2048/3096
 * @return unsigned int the private key bytes
 */
unsigned int paillier_max_private_key_bytes(int key_bits);

/**
 * @brief get the paillier max cipher bytes according to the keyBits
 *
 * @param key_bits e.g. 512/1024/2048/3096
 * @return unsigned int the cipher bytes
 */
unsigned int paillier_max_cipher_bytes(int key_bits);


/**
 * @brief get the paillier max cipher bytes according to the keypair
 *
 * @param key_pair the key pair object
 * @return unsigned int
 */
unsigned int paillier_key_bits_from_keypair(void* key_pair);

/**
 * @brief obtain n from the given keyPair
 *
 * @param n
 * @param pk
 */
void paillier_n_from_pk(BIGNUM* n, void* pk);

/**
 * @brief get the paillier max cipher bytes according to the  public key
 *
 * @param public_key the public key object
 * @return unsigned int
 */
unsigned int paillier_key_bits_from_public_key(void* public_key);

/**
 * @brief generate the paillier KeyPair
 *
 * @return void* pointer to the generated paillier key-pair
 */
void* paillier_generate_keypair(int key_bits);

/**
 * @brief load the encoded privateKey and publicKey into key-pair object
 *
 * @param sk the encoded privateKey
 * @param pk the encoded publicKey
 * @return void* void* pointer to the loaded paillier key-pair
 */
void* paillier_load_keypair(InputBuffer const* sk, InputBuffer const* pk);

/**
 * @brief free the allocated keypair
 *
 * @param keypair
 */
void paillier_free_key_pair(void* keypair);

/**
 * @brief get the public key object according to the keypair
 *
 * @param keypair
 * @return void*
 */
void* paillier_get_public_key(void* keypair);

/**
 * @brief load the publicKey object from the encoded publicKey
 *
 * @param pk the encoded publicKey
 * @return void* the pointer to the public key object
 */
void* paillier_load_public_key(InputBuffer const* pk);


/**
 * @brief free the allocated public key
 *
 * @param public_key
 */
void paillier_free_public_key(void* public_key);

/**
 * @brief load the private key according to the keyBytes
 *
 * @param sk
 * @return void*
 */
void* paillier_load_private_key(InputBuffer const* sk);

/**
 * @brief free the private key
 *
 * @param private_key
 */
void paillier_free_private_key(void* private_key);

/**
 * @brief get the encoded publicKey from given keyPair object
 *
 * @param pkBytes return the encoded public key
 *  Note: should allocate OutputBuffer before call this function, the lifecycle of OutputBuffer is
 * managed by the caller(maybe cross-programming language)
 * @param keypair the pointer to the keyPair object
 */
void paillier_set_public_key_bytes_from_keypair(OutputBuffer* pkBytes, void* keypair);

/**
 * @brief get the encoded publicKey from given keyPair object
 *
 * @param keypair the pointer to the keyPair object
 * @return OutputBuffer the public key-bytes
 *        Note: the caller should release the allocated buffer
 */
OutputBuffer paillier_get_public_key_bytes_from_keyPair(void* keypair);


/**
 * @brief Get the private key bytes object
 *
 * @param pkBytes return the encoded private key
 *  Note: should allocate OutputBuffer before call this function, the lifecycle of OutputBuffer is
 * managed by the caller(maybe cross-programming language)
 * @param keypair the pointer to the keyPair object
 */
void paillier_set_private_key_bytes_from_keypair(OutputBuffer* skBytes, void* keypair);

/**
 * @brief Get the private key bytes
 *
 * @param keypair the pointer to the keyPair object
 * @return OutputBuffer the private key-bytes
 *        Note: the caller should release the allocated buffer
 */
OutputBuffer paillier_get_private_key_bytes_from_keypair(void* keypair);

/**
 * @brief encrypt the value into paillier cipher using given keyPair with CRT optimization
 *
 * @param cipherBytes return the encrypted cipher
 * @param value the value to be encrypted
 * @param keypair the keyPair used to encrypt
 */
void paillier_encryt_fast(
    OutputBuffer* cipherBytes, OutputBuffer* rBytes, BIGNUM* value, void* keypair);
void paillier_encryt_fast_without_precompute(OutputBuffer* cipherBytes, OutputBuffer* rBytes,
    BIGNUM* value, InputBuffer const* sk, InputBuffer const* pk);

/**
 * @brief encrypt the value into paillier cipher using given keyPair without CRT optimization
 *
 * @param cipherBytes return the encrypted cipher
 * @param value the value to be encrypted
 * @param publicKey the keyPair used to encrypt
 */
void paillier_encryt(
    OutputBuffer* cipherBytes, OutputBuffer* rBytes, BIGNUM* value, void* publicKey);
void paillier_encryt_without_precompute(
    OutputBuffer* cipherBytes, OutputBuffer* rBytes, BIGNUM* value, InputBuffer const* pk);

/**
 * @brief decrypt the cipher using given keyPair
 *
 * @param cipher the cipher to be decrypted
 * @param keypair the keyPair used to decrypt
 * @return BIGNUM* the decrypted result
 * Note: the caller should release the allocated BIGNUM
 */
BIGNUM* paillier_decrypt(InputBuffer const* cipher, void* keypair);
BIGNUM* paillier_decrypt_without_precompute(
    InputBuffer const* cipher, InputBuffer const* sk, InputBuffer const* pk);

/**
 * @brief paillier ciphertext space addition, namely: cipher1 + cipher2
 *
 * @param cipher1
 * @param cipher2
 * @param public_key
 * @return void* the addition result
 */
void paillier_add(OutputBuffer* cipherBytes, InputBuffer const* cipher1, InputBuffer const* cipher2,
    void* public_key);
void paillier_add_without_precompute(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, InputBuffer const* pk);

/**
 * @brief paillier ciphertext space addition, namely: cipher1 - cipher2
 *
 * @param cipher1
 * @param cipher2
 * @param public_key
 * @return void* the subtraction result
 */
void paillier_sub(OutputBuffer* cipherBytes, InputBuffer const* cipher1, InputBuffer const* cipher2,
    void* public_key);
void paillier_sub_without_precompute(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, InputBuffer const* pkBytes);

/**
 * @brief paillier ciphertext space scala-multiply, namely: v * cipher
 *
 * @param v the scala-number
 * Note: the lifecycle of value is managed by the caller
 * @param cipher
 * @param public_key
 * @return void* the scala-multiply result
 */
void paillier_scala_mul(
    OutputBuffer* cipherBytes, BIGNUM* v, InputBuffer const* cipher, void* public_key);
void paillier_scala_mul_without_precompute(
    OutputBuffer* cipherBytes, BIGNUM* v, InputBuffer const* cipher, InputBuffer const* pkBytes);

#ifdef __cplusplus
}
#endif
#endif
