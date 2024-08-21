/**
 *  Copyright (C) 2022 WeDPR.
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
 * @file OpenSSLAES.cpp
 * @author: shawnhe
 * @date 2022-11-29
 */

#include "OpenSSLAES.h"

using namespace ppc::crypto;
using namespace ppc::protocol;
EvpCipherPtr OpenSSLAES::createCipherMeth(OperationMode _mode) const
{
    if (m_aesType == AESType::AES128)
    {
        switch (_mode)
        {
        case SymCrypto::OperationMode::ECB:
            return EvpCipherPtr(EVP_aes_128_ecb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CBC:
            return EvpCipherPtr(EVP_aes_128_cbc(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CFB:
            return EvpCipherPtr(EVP_aes_128_cfb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::OFB:
            return EvpCipherPtr(EVP_aes_128_ofb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CTR:
            return EvpCipherPtr(EVP_aes_128_ctr(), EvpCipherDeleter());
        default:
            BOOST_THROW_EXCEPTION(
                SymCryptoException() << bcos::errinfo_comment(
                    "unsupported mode for AES128: " + std::to_string(int(_mode))));
        }
    }
    else if (m_aesType == AESType::AES192)
    {
        switch (_mode)
        {
        case SymCrypto::OperationMode::ECB:
            return EvpCipherPtr(EVP_aes_192_ecb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CBC:
            return EvpCipherPtr(EVP_aes_192_cbc(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CFB:
            return EvpCipherPtr(EVP_aes_192_cfb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::OFB:
            return EvpCipherPtr(EVP_aes_192_ofb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CTR:
            return EvpCipherPtr(EVP_aes_192_ctr(), EvpCipherDeleter());
        default:
            BOOST_THROW_EXCEPTION(
                SymCryptoException() << bcos::errinfo_comment(
                    "unsupported mode for AES192: " + std::to_string(int(_mode))));
        }
    }
    else if (m_aesType == AESType::AES256)
    {
        switch (_mode)
        {
        case SymCrypto::OperationMode::ECB:
            return EvpCipherPtr(EVP_aes_256_ecb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CBC:
            return EvpCipherPtr(EVP_aes_256_cbc(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CFB:
            return EvpCipherPtr(EVP_aes_256_cfb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::OFB:
            return EvpCipherPtr(EVP_aes_256_ofb(), EvpCipherDeleter());
        case SymCrypto::OperationMode::CTR:
            return EvpCipherPtr(EVP_aes_256_ctr(), EvpCipherDeleter());
        default:
            BOOST_THROW_EXCEPTION(
                SymCryptoException() << bcos::errinfo_comment(
                    "unsupported mode for AES256: " + std::to_string(int(_mode))));
        }
    }
    BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                              "Unsupported aes-algorithm: " + std::to_string((int)m_aesType)));
}
