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
 * @file symmetric_encryption.cpp
 * @author: yujiechen
 * @date 2023-09-08
 */
#include "symmetric_encryption.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSL3DES.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLAES.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLSM4.h"
#include "utils/error.h"
#include <bcos-utilities/Log.h>
#include <thread>

using namespace ppc;
using namespace ppc::crypto;

thread_local OpenSSLSM4::Ptr g_sm4_impl = std::make_shared<OpenSSLSM4>();
thread_local OpenSSL3DES::Ptr g_3des_impl = std::make_shared<OpenSSL3DES>();
thread_local OpenSSLAES::Ptr g_aes128_impl =
    std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128);
thread_local OpenSSLAES::Ptr g_aes192_impl =
    std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES192);
thread_local OpenSSLAES::Ptr g_aes256_impl =
    std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES256);


SymCrypto::Ptr obtainSymCryptoImpl(int algorithm)
{
    switch (algorithm)
    {
    case AlgorithmType::AES_128:
        return g_aes128_impl;
    case AlgorithmType::AES_192:
        return g_aes192_impl;
    case AlgorithmType::AES_256:
        return g_aes256_impl;
    case AlgorithmType::TrippleDES:
        return g_3des_impl;
    case AlgorithmType::SM4:
        return g_sm4_impl;
    default:
        return nullptr;
    }
}

unsigned int symmetric_block_size(int algorithm)
{
    clear_last_error();
    auto impl = obtainSymCryptoImpl(algorithm);
    if (!impl)
    {
        auto errorMsg =
            "symmetric_block_size error for unsupported algorithm: " + std::to_string(algorithm);
        set_last_error_msg(-1, errorMsg.c_str());
        return -1;
    }
    return impl->blockSize();
}

int symmetric_key_bytes(int algorithm, int mode)
{
    clear_last_error();
    try
    {
        auto impl = obtainSymCryptoImpl(algorithm);
        if (!impl)
        {
            auto errorMsg =
                "symmetric_key_bytes error for unsupported algorithm: " + std::to_string(algorithm);
            set_last_error_msg(-1, errorMsg.c_str());
            return -1;
        }
        return impl->keyBytes((SymCrypto::OperationMode)mode);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
    return -1;
}
/**
 * @brief generate key for symmetric encryption
 *
 * @param _sk the generated key
 */
void symmetric_generate_key(OutputBuffer* _sk, int algorithm, int mode)
{
    clear_last_error();
    try
    {
        auto impl = obtainSymCryptoImpl(algorithm);
        if (!impl)
        {
            auto errorMsg = "Unsupported algorithm: " + std::to_string(algorithm);
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }
        impl->generateKey(_sk, (SymCrypto::OperationMode)mode);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

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
    InputBuffer const* iv, InputBuffer const* plainData)
{
    clear_last_error();
    try
    {
        auto impl = obtainSymCryptoImpl(algorithm);
        if (!impl)
        {
            auto errorMsg =
                "symmetric_encrypt error: unsupported algorithm " + std::to_string(algorithm);
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }
        impl->encrypt(cipher, (SymCrypto::OperationMode)mode,
            bcos::bytesConstRef(sk->data, sk->len), bcos::bytesConstRef(iv->data, iv->len),
            bcos::bytesConstRef(plainData->data, plainData->len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

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
    InputBuffer const* iv, InputBuffer const* cipher)
{
    clear_last_error();
    try
    {
        auto impl = obtainSymCryptoImpl(algorithm);
        if (!impl)
        {
            auto errorMsg =
                "symmetric_decrypt failed for unsupported algorithm: " + std::to_string(algorithm);
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }
        impl->decrypt(plain, (SymCrypto::OperationMode)mode, bcos::bytesConstRef(sk->data, sk->len),
            bcos::bytesConstRef(iv->data, iv->len), bcos::bytesConstRef(cipher->data, cipher->len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
