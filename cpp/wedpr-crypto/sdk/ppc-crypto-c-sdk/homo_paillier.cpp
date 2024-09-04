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
 * @file homo_paillier.cpp
 * @author: yujiechen
 * @date 2023-08-11
 */
#include "homo_paillier.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"
#include "ppc-homo/paillier/OpenSSLPaillierKeyPair.h"
#include "utils/error.h"
#include <bcos-utilities/Log.h>
#include <thread>

using namespace ppc::homo;
using namespace ppc::crypto;
using namespace bcos;

thread_local OpenSSLPaillier::Ptr g_paillier_impl = std::make_shared<OpenSSLPaillier>();

unsigned int paillier_max_public_key_bytes(int key_bits)
{
    clear_last_error();
    return ppc::homo::PaillierPublicKey::maxBytes(key_bits);
}

unsigned int paillier_r_bytes_len(int key_bits)
{
    clear_last_error();
    return ppc::homo::OpenSSLPaillier::rBytesLen(key_bits);
}

unsigned int paillier_max_private_key_bytes(int key_bits)
{
    clear_last_error();
    return ppc::homo::PaillierPrivateKey::maxBytes(key_bits);
}

unsigned int paillier_max_cipher_bytes(int key_bits)
{
    clear_last_error();
    return ppc::homo::OpenSSLPaillier::maxCipherBytesLen(key_bits);
}

unsigned int paillier_key_bits_from_keypair(void* key_pair)
{
    clear_last_error();
    auto keyPair = (ppc::homo::OpenSSLPaillierKeyPair*)(key_pair);
    auto pk = (ppc::homo::PaillierPublicKey*)(keyPair->pk());
    return pk->keyBits;
}

unsigned int paillier_key_bits_from_public_key(void* public_key)
{
    clear_last_error();
    auto pk = (ppc::homo::PaillierPublicKey*)(public_key);
    return pk->keyBits;
}

/**
 * @brief obtain n from the given keyPair
 *
 * @param n
 * @param pk
 * @return void
 */
void paillier_n_from_pk(BIGNUM* n, void* pk)
{
    clear_last_error();
    auto pkPtr = (PaillierPublicKey*)pk;
    pkPtr->n.copy(n);
}

/**
 * @brief generate the paillier KeyPair
 *
 * @return void* pointer to the generated paillier key-pair
 */
void* paillier_generate_keypair(int key_bits)
{
    clear_last_error();
    try
    {
        auto keypair = g_paillier_impl->generateKeyPair(key_bits);
        return keypair.release();
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return nullptr;
    }
}

/**
 * @brief load the encoded privateKey and publicKey into key-pair object
 *
 * @param sk the encoded privateKey
 * @param pk the encoded publicKey
 * @return void* void* pointer to the loaded paillier key-pair
 */
void* paillier_load_keypair(InputBuffer const* sk, InputBuffer const* pk)
{
    clear_last_error();
    try
    {
        auto keyPair =
            std::make_unique<OpenSSLPaillierKeyPair>(sk->data, sk->len, pk->data, pk->len);
        return keyPair.release();
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return nullptr;
    }
}


/**
 * @brief free the allocated keypair
 *
 * @param keypair
 */
void paillier_free_key_pair(void* keypair)
{
    clear_last_error();
    if (keypair)
    {
        free(keypair);
    }
}

/**
 * @brief get the public key object according to the keypair
 *
 * @param keypair
 * @return void*
 */
void* paillier_get_public_key(void* keypair)
{
    clear_last_error();
    auto paillierKeyPair = (OpenSSLPaillierKeyPair*)keypair;
    return paillierKeyPair->pk();
}
/**
 * @brief load the publicKey object from the encoded publicKey
 *
 * @param pk the encoded publicKey
 * @return void* the pointer to the public key object
 */
void* paillier_load_public_key(InputBuffer const* pkBytes)
{
    clear_last_error();
    try
    {
        auto pk = std::make_unique<PaillierPublicKey>(pkBytes->data, pkBytes->len);
        return pk.release();
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return nullptr;
    }
}

/**
 * @brief free the allocated public key
 *
 * @param public_key
 */
void paillier_free_public_key(void* public_key)
{
    clear_last_error();
    if (public_key)
    {
        free(public_key);
    }
}

/**
 * @brief free the private key
 *
 * @param private_key
 */
void paillier_free_private_key(void* private_key)
{
    clear_last_error();
    if (private_key)
    {
        free(private_key);
    }
}

/**
 * @brief load the private key according to the keyBytes
 *
 * @param sk
 * @return void*
 */
void* paillier_load_private_key(InputBuffer const* skBytes)
{
    clear_last_error();
    try
    {
        auto sk = std::make_unique<PaillierPrivateKey>(skBytes->data, skBytes->len);
        return sk.release();
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return nullptr;
    }
}

/**
 * @brief get the encoded publicKey from given keyPair object
 *
 * @param pkBytes return the encoded public key
 *  Note: should allocate OutputBuffer before call this function, the lifecycle of OutputBuffer is
 * managed by the caller(maybe cross-programming language)
 * @param keypair the pointer to the keyPair object
 */
void paillier_set_public_key_bytes_from_keypair(OutputBuffer* pkBytes, void* keypair)
{
    clear_last_error();
    try
    {
        auto paillierKeyPair = (OpenSSLPaillierKeyPair*)(keypair);
        auto pk = (PaillierPublicKey*)(paillierKeyPair->pk());
        pkBytes->len = pk->serialize((bcos::byte*)pkBytes->data, pkBytes->len);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

OutputBuffer paillier_get_public_key_bytes_from_keyPair(void* keypair)
{
    clear_last_error();
    unsigned char* publicKeyBytes = NULL;
    try
    {
        auto paillierKeyPair = (OpenSSLPaillierKeyPair*)(keypair);
        auto pk = (PaillierPublicKey*)(paillierKeyPair->pk());

        auto bufferLen = paillier_max_public_key_bytes(pk->keyBits);
        // Note: this should be released by the caller after used
        publicKeyBytes = (unsigned char*)malloc(bufferLen);
        OutputBuffer publicKeyBuffer{publicKeyBytes, bufferLen};
        paillier_set_public_key_bytes_from_keypair(&publicKeyBuffer, keypair);
        return publicKeyBuffer;
    }
    catch (std::exception const& e)
    {
        if (!publicKeyBytes)
        {
            free(publicKeyBytes);
        }
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return OutputBuffer{NULL, 0};
    }
}

/**
 * @brief Get the private key bytes object
 *
 * @param keypair the pointer to the keyPair object
 * @return void* the encoded private key
 */
void paillier_set_private_key_bytes_from_keypair(OutputBuffer* skBytes, void* keypair)
{
    clear_last_error();
    try
    {
        auto paillierKeyPair = (OpenSSLPaillierKeyPair*)(keypair);
        auto sk = (PaillierPrivateKey*)(paillierKeyPair->sk());
        skBytes->len = sk->serialize((bcos::byte*)skBytes->data, skBytes->len);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief Get the private key bytes
 *
 * @param keypair the pointer to the keyPair object
 * @return OutputBuffer the private key-bytes
 *        Note: the caller should release the allocated buffer
 */
OutputBuffer paillier_get_private_key_bytes_from_keypair(void* keypair)
{
    clear_last_error();
    unsigned char* skBytes = NULL;
    try
    {
        auto paillierKeyPair = (OpenSSLPaillierKeyPair*)(keypair);
        auto sk = (PaillierPrivateKey*)(paillierKeyPair->sk());
        auto bufferLen = paillier_max_private_key_bytes(sk->keyBits);

        // Note: this should be released by the caller after used
        skBytes = (unsigned char*)malloc(bufferLen);
        OutputBuffer skBuffer{skBytes, bufferLen};
        paillier_set_private_key_bytes_from_keypair(&skBuffer, keypair);
        return skBuffer;
    }
    catch (std::exception const& e)
    {
        if (skBytes)
        {
            free(skBytes);
        }
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return OutputBuffer{NULL, 0};
    }
}

/**
 * @brief encrypt the value into paillier cipher using given keyPair with CRT optimization
 *
 * @param cipherBytes return the encrypted cipher
 * @param value the value to be encrypted
 * @param keypair the keyPair used to encrypt
 */
void paillier_encryt_fast(
    OutputBuffer* cipherBytes, OutputBuffer* rBytes, BIGNUM* value, void* keypair)
{
    clear_last_error();
    try
    {
        g_paillier_impl->encrypt_with_crt(cipherBytes, rBytes, value, keypair);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void paillier_encryt_fast_without_precompute(OutputBuffer* cipherBytes, OutputBuffer* rBytes,
    BIGNUM* value, InputBuffer const* sk, InputBuffer const* pk)
{
    auto keyPair = paillier_load_keypair(sk, pk);
    if (!last_call_success())
    {
        return;
    }

    paillier_encryt_fast(cipherBytes, rBytes, value, keyPair);
    paillier_free_key_pair(keyPair);
}

/**
 * @brief encrypt the value into paillier cipher using given keyPair without CRT optimization
 *
 * @param cipherBytes return the encrypted cipher
 * @param value the value to be encrypted
 * @param keypair the keyPair used to encrypt
 */
void paillier_encryt(
    OutputBuffer* cipherBytes, OutputBuffer* rBytes, BIGNUM* value, void* publicKey)
{
    clear_last_error();
    try
    {
        g_paillier_impl->encrypt(cipherBytes, rBytes, value, publicKey);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
void paillier_encryt_without_precompute(
    OutputBuffer* cipherBytes, OutputBuffer* rBytes, BIGNUM* value, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    paillier_encryt(cipherBytes, rBytes, value, pk);
    paillier_free_public_key(pk);
}

/**
 * @brief decrypt the cipher using given keyPair
 *
 * @param cipher the cipher to be decrypted
 * @param keypair the keyPair used to decrypt
 * @return BIGNUM* the decrypted result
 * Note: the caller should release the allocated BIGNUM
 */
BIGNUM* paillier_decrypt(InputBuffer const* cipher, void* keypair)
{
    clear_last_error();
    try
    {
        auto decryptResult = g_paillier_impl->decrypt(
            bcos::bytesConstRef((bcos::byte const*)cipher->data, cipher->len), keypair);
        BIGNUM* ret = BN_new();
        // swap the result
        BN_swap(ret, decryptResult.bn().get());
        return ret;
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
        return NULL;
    }
}

BIGNUM* paillier_decrypt_without_precompute(
    InputBuffer const* cipher, InputBuffer const* sk, InputBuffer const* pk)
{
    auto keyPair = paillier_load_keypair(sk, pk);
    if (!last_call_success())
    {
        return NULL;
    }
    auto result = paillier_decrypt(cipher, keyPair);
    paillier_free_key_pair(keyPair);
    return result;
}

/**
 * @brief paillier ciphertext space addition, namely: cipher1 + cipher2
 *
 * @param cipher1
 * @param cipher2
 * @param public_key
 * @return void* the addition result
 */
void paillier_add(OutputBuffer* cipherBytes, InputBuffer const* cipher1, InputBuffer const* cipher2,
    void* public_key)
{
    clear_last_error();
    try
    {
        auto cipher1Ref = bcos::bytesConstRef(cipher1->data, cipher1->len);
        auto cipher2Ref = bcos::bytesConstRef(cipher2->data, cipher2->len);
        g_paillier_impl->add(cipherBytes, cipher1Ref, cipher2Ref, public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
void paillier_add_without_precompute(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    paillier_add(cipherBytes, cipher1, cipher2, pk);
    paillier_free_public_key(pk);
}
/**
 * @brief paillier ciphertext space addition, namely: cipher1 - cipher2
 *
 * @param cipher1
 * @param cipher2
 * @param public_key
 * @return void* the subtraction result
 */
void paillier_sub(OutputBuffer* cipherBytes, InputBuffer const* cipher1, InputBuffer const* cipher2,
    void* public_key)
{
    clear_last_error();
    try
    {
        auto cipher1Ref = bcos::bytesConstRef(cipher1->data, cipher1->len);
        auto cipher2Ref = bcos::bytesConstRef(cipher2->data, cipher2->len);
        g_paillier_impl->sub(cipherBytes, cipher1Ref, cipher2Ref, public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void paillier_sub_without_precompute(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    paillier_sub(cipherBytes, cipher1, cipher2, pk);
    paillier_free_public_key(pk);
}
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
    OutputBuffer* cipherBytes, BIGNUM* v, InputBuffer const* cipher, void* public_key)
{
    clear_last_error();
    try
    {
        auto cipherRef = bcos::bytesConstRef(cipher->data, cipher->len);
        g_paillier_impl->scalaMul(cipherBytes, v, cipherRef, public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void paillier_scala_mul_without_precompute(
    OutputBuffer* cipherBytes, BIGNUM* v, InputBuffer const* cipher, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    paillier_scala_mul(cipherBytes, v, cipher, pk);
    paillier_free_public_key(pk);
}