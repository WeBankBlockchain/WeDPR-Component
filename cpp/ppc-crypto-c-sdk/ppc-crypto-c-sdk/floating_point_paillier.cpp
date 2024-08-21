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
 * @file floating_point_paillier.cpp
 * @author: yujiechen
 * @date 2023-08-20
 */
#include "floating_point_paillier.h"
#include "homo_paillier.h"
#include "ppc-homo/paillier/FloatingPointPaillier.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"
#include "utils/error.h"

using namespace ppc;
using namespace ppc::homo;
using namespace ppc::crypto;
using namespace bcos;

OpenSSLPaillier::Ptr g_paillier_ptr = std::make_shared<OpenSSLPaillier>();
thread_local FloatingPointPaillier::Ptr g_floating_point_paillier =
    std::make_shared<FloatingPointPaillier>(g_paillier_ptr);

unsigned int floating_point_paillier_cipher_bytes(unsigned int keyBits)
{
    clear_last_error();
    return FloatingPointPaillier::maxCipherBytesLen(keyBits);
}
/**
 * @brief encrypt {value*10^exponent} using paillier crt-optimization
 *
 * @param cipherBytes the encrypted result
 * @param value the value to be encrypted
 * @param exponent the exponent of the value to be encrypted
 * @param keypair the keypair used to encrypt
 */
void floating_point_paillier_encrypt_fast(
    OutputBuffer* cipherBytes, BIGNUM const* value, int16_t exponent, void* keypair)
{
    clear_last_error();
    try
    {
        BigNum v;
        BN_copy(v.bn().get(), value);
        FloatingPointNumber floatingPointV(std::move(v), exponent);
        g_floating_point_paillier->encryptFast(cipherBytes, floatingPointV, keypair);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void floating_point_paillier_encrypt_fast_without_precompute(OutputBuffer* cipherBytes,
    BIGNUM const* value, int16_t exponent, InputBuffer const* sk, InputBuffer const* pkBytes)
{
    auto keyPair = paillier_load_keypair(sk, pkBytes);
    if (!last_call_success())
    {
        return;
    }
    floating_point_paillier_encrypt_fast(cipherBytes, value, exponent, keyPair);
    paillier_free_key_pair(keyPair);
}

/**
 * @brief encrypt {value*10^exponent} withou using paillier crt-optimization
 *
 * @param cipherBytes the encrypted result
 * @param value the value to be encrypted
 * @param exponent the exponent of the value to be encrypted
 * @param public_key the public_key used to encrypt
 */
void floating_point_paillier_encrypt(
    OutputBuffer* cipherBytes, BIGNUM const* value, int16_t exponent, void* public_key)
{
    clear_last_error();
    try
    {
        BigNum v;
        BN_copy(v.bn().get(), value);
        FloatingPointNumber floatingPointV(std::move(v), exponent);
        g_floating_point_paillier->encrypt(cipherBytes, floatingPointV, public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
void floating_point_paillier_encrypt_without_precompute(
    OutputBuffer* cipherBytes, BIGNUM const* value, int16_t _exponent, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    floating_point_paillier_encrypt(cipherBytes, value, _exponent, pk);
    paillier_free_public_key(pk);
}
/**
 * @brief paillier decrypt
 *
 * @param value the decrypted value
 * @param exponent the exponent of the decrypted value
 * @param cipher the cipher to be decrypted
 * @param keypair the keyPair used to decrypt
 */
void floating_point_paillier_decrypt(
    BIGNUM* value, int16_t* exponent, InputBuffer const* cipher, void* keypair)
{
    clear_last_error();
    try
    {
        auto ret = g_floating_point_paillier->decrypt(
            bcos::bytesConstRef(cipher->data, cipher->len), keypair);
        // swap the result to value
        ret.value.swap(value);
        *exponent = ret.exponent;
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void floating_point_paillier_decrypt_without_precompute(BIGNUM* value, int16_t* exponent,
    InputBuffer const* cipher, InputBuffer const* sk, InputBuffer const* pkBytes)
{
    auto keyPair = paillier_load_keypair(sk, pkBytes);
    if (!last_call_success())
    {
        return;
    }
    floating_point_paillier_decrypt(value, exponent, cipher, keyPair);
    paillier_free_key_pair(keyPair);
}

/**
 * @brief paillier add
 *
 * @param cipherBytes the paillier cipher add result
 * @param cipher1
 * @param cipher2
 * @param public_key
 */
void floating_point_paillier_add(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, void* public_key)
{
    clear_last_error();
    try
    {
        g_floating_point_paillier->add(cipherBytes,
            bcos::bytesConstRef(cipher1->data, cipher1->len),
            bcos::bytesConstRef(cipher2->data, cipher2->len), public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
void floating_point_paillier_add_without_precompute(OutputBuffer* cipherBytes,
    InputBuffer const* cipher1, InputBuffer const* cipher2, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    floating_point_paillier_add(cipherBytes, cipher1, cipher2, pk);
    paillier_free_public_key(pk);
}

/**
 * @brief paillier sub
 *
 * @param cipherBytes the paillier cipher sub result
 * @param cipher1
 * @param cipher2
 * @param public_key
 */
void floating_point_paillier_sub(OutputBuffer* cipherBytes, InputBuffer const* cipher1,
    InputBuffer const* cipher2, void* public_key)
{
    clear_last_error();
    try
    {
        g_floating_point_paillier->sub(cipherBytes,
            bcos::bytesConstRef(cipher1->data, cipher1->len),
            bcos::bytesConstRef(cipher2->data, cipher2->len), public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void floating_point_paillier_sub_without_precompute(OutputBuffer* cipherBytes,
    InputBuffer const* cipher1, InputBuffer const* cipher2, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    floating_point_paillier_sub(cipherBytes, cipher1, cipher2, pk);
    paillier_free_public_key(pk);
}

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
    InputBuffer const* cipher, void* public_key)
{
    clear_last_error();
    try
    {
        BigNum value;
        BN_copy(value.bn().get(), v);
        FloatingPointNumber floatingPointV(std::move(value), exponent);
        g_floating_point_paillier->scalaMul(cipherBytes, floatingPointV,
            bcos::bytesConstRef(cipher->data, cipher->len), public_key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void floating_point_paillier_scalaMul_without_precompute(OutputBuffer* cipherBytes, BIGNUM const* v,
    int16_t exponent, InputBuffer const* cipher, InputBuffer const* pkBytes)
{
    auto pk = paillier_load_public_key(pkBytes);
    if (!last_call_success())
    {
        return;
    }
    floating_point_paillier_scalaMul(cipherBytes, v, exponent, cipher, pk);
    paillier_free_public_key(pk);
}