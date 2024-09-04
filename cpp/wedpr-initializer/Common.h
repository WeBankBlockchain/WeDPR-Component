/*
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
 * @file Common.h
 * @author: yujiechen
 * @date 2022-11-14
 */

#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "ppc-tools/src/config/Common.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/FileUtility.h>
#include <bcos-utilities/Log.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>

#define INIT_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("Initializer")

namespace ppc::initializer
{
inline bcos::bytes loadPrivateKey(std::string const& _keyPath, unsigned _hexedPrivateKeySize)
{
    auto keyContent = bcos::readContents(boost::filesystem::path(_keyPath));
    std::shared_ptr<EC_KEY> ecKey;
    try
    {
        std::shared_ptr<BIO> bioMem(BIO_new(BIO_s_mem()), [&](BIO* p) { BIO_free(p); });
        BIO_write(bioMem.get(), keyContent->data(), keyContent->size());

        std::shared_ptr<EVP_PKEY> evpPKey(PEM_read_bio_PrivateKey(bioMem.get(), NULL, NULL, NULL),
            [](EVP_PKEY* p) { EVP_PKEY_free(p); });
        if (!evpPKey)
        {
            BOOST_THROW_EXCEPTION(
                ppc::tools::InvalidConfig() << bcos::errinfo_comment(
                    "SecureInitializer: parse privateKey failed for empty EVP_PKEY!"));
        }
        ecKey.reset(EVP_PKEY_get1_EC_KEY(evpPKey.get()), [](EC_KEY* p) { EC_KEY_free(p); });
    }
    catch (std::exception const& e)
    {
        INIT_LOG(ERROR) << LOG_BADGE("SecureInitializer") << LOG_DESC("parse privateKey failed")
                        << LOG_KV("EINFO", boost::diagnostic_information(e));
        BOOST_THROW_EXCEPTION(ppc::tools::InvalidConfig() << bcos::errinfo_comment(
                                  "SecureInitializer: parse privateKey failed, error: " +
                                  std::string(boost::diagnostic_information(e))));
    }
    std::shared_ptr<const BIGNUM> ecPrivateKey(
        EC_KEY_get0_private_key(ecKey.get()), [](const BIGNUM*) {});

    std::shared_ptr<char> privateKeyData(
        BN_bn2hex(ecPrivateKey.get()), [](char* p) { OPENSSL_free(p); });
    std::string keyHex(privateKeyData.get());
    if (keyHex.size() >= _hexedPrivateKeySize)
    {
        return *(bcos::fromHexString(keyHex));
    }
    for (size_t i = keyHex.size(); i < _hexedPrivateKeySize; i++)
    {
        keyHex = '0' + keyHex;
    }
    return *(bcos::fromHexString(keyHex));
}
}  // namespace ppc::initializer
#pragma GCC diagnostic pop