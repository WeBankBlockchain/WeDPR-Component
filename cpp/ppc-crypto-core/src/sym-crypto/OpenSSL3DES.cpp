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
 * @file OpenSSL3DES.cpp
 * @author: shawnhe
 * @date 2023-08-16
 */

#include "OpenSSL3DES.h"

using namespace ppc::crypto;
using namespace ppc::protocol;

EvpCipherPtr OpenSSL3DES::createCipherMeth(OperationMode _mode) const
{
    switch (_mode)
    {
    case SymCrypto::OperationMode::ECB:
        return EvpCipherPtr(EVP_des_ede3_ecb(), EvpCipherDeleter());
    case SymCrypto::OperationMode::CBC:
        return EvpCipherPtr(EVP_des_ede3_cbc(), EvpCipherDeleter());
    case SymCrypto::OperationMode::CFB:
        return EvpCipherPtr(EVP_des_ede3_cfb(), EvpCipherDeleter());
    case SymCrypto::OperationMode::OFB:
        return EvpCipherPtr(EVP_des_ede3_ofb(), EvpCipherDeleter());
    default:
        BOOST_THROW_EXCEPTION(SymCryptoException() << bcos::errinfo_comment(
                                  "unsupported crypto mode: " + std::to_string(int(_mode))));
    }
}