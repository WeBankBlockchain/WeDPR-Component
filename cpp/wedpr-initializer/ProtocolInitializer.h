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
 * @file ProtocolInitializer.h
 * @author: yujiechen
 * @date 2022-11-14
 */
#pragma once
#include "Common.h"
#include "ppc-crypto-core/src/hash/MD5Hash.h"
#include "ppc-crypto-core/src/hash/SM3Hash.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/protocol/GlobalConfig.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-storage/src/FileStorageFactoryImpl.h"
#include "ppc-storage/src/SQLStorageFactoryImpl.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::initializer
{
class ProtocolInitializer
{
public:
    using Ptr = std::shared_ptr<ProtocolInitializer>;
    ProtocolInitializer()
      : m_ppcMsgFactory(std::make_shared<ppc::front::PPCMessageFactory>()),
        m_sqlStorageFactory(std::make_shared<ppc::storage::SQLStorageFactoryImpl>()),
        m_fileStorageFactory(std::make_shared<ppc::storage::FileStorageFactoryImpl>())
    {}

    virtual ~ProtocolInitializer() = default;

    virtual void init(ppc::tools::PPCConfig::Ptr _config)
    {
        m_dataResourceLoader = std::make_shared<ppc::io::DataResourceLoaderImpl>(
            _config->storageConfig().sqlConnectionOpt,
            _config->storageConfig().fileStorageConnectionOpt,
            _config->storageConfig().remoteConnectionOpt, m_sqlStorageFactory, m_fileStorageFactory,
            m_remoteStorageFactory);

        g_PPCConfig.setSMCrypto(_config->smCrypto());
        if (!_config->smCrypto())
        {
            auto hashImpl = std::make_shared<ppc::crypto::Sha256Hash>();
            auto eccCrypto = std::make_shared<ppc::crypto::OpenSSLEccCrypto>(
                hashImpl, ppc::protocol::ECCCurve::P256);
            m_cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(hashImpl, eccCrypto);
            m_binHashImpl = std::make_shared<ppc::crypto::MD5Hash>();
        }
        else
        {
            auto hashImpl = std::make_shared<ppc::crypto::SM3Hash>();
            auto eccCrypto = std::make_shared<ppc::crypto::OpenSSLEccCrypto>(
                hashImpl, ppc::protocol::ECCCurve::SM2);
            m_cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(hashImpl, eccCrypto);
            m_binHashImpl = hashImpl;
        }
        INIT_LOG(INFO) << LOG_DESC("loadPrivateKey")
                       << LOG_KV("privateKeyPath", _config->privateKeyPath());
        auto privateKey = loadPrivateKey(_config->privateKeyPath(), PRIVATE_KEY_HEX_LEN);
        _config->setPrivateKey(privateKey);
        INIT_LOG(INFO) << LOG_DESC("loadPrivateKey success");
    }

    ppc::front::PPCMessageFaceFactory::Ptr const& ppcMsgFactory() const { return m_ppcMsgFactory; }
    ppc::crypto::CryptoBox::Ptr const& cryptoBox() const { return m_cryptoBox; };
    ppc::crypto::Hash::Ptr const& binHashImpl() const { return m_binHashImpl; };

    ppc::storage::SQLStorageFactory::Ptr const& sqlStorageFactory() const
    {
        return m_sqlStorageFactory;
    }
    ppc::io::DataResourceLoader::Ptr const& dataResourceLoader() const
    {
        return m_dataResourceLoader;
    }

    ppc::storage::FileStorageFactory::Ptr const& fileStorageFactory() const
    {
        return m_fileStorageFactory;
    }

    ppc::storage::RemoteStorageFactory::Ptr const& remoteStorageFactory() const
    {
        return m_remoteStorageFactory;
    }

private:
    ppc::front::PPCMessageFaceFactory::Ptr m_ppcMsgFactory;
    ppc::storage::SQLStorageFactory::Ptr m_sqlStorageFactory;
    ppc::storage::FileStorageFactory::Ptr m_fileStorageFactory;
    ppc::storage::RemoteStorageFactory::Ptr m_remoteStorageFactory;
    ppc::io::DataResourceLoader::Ptr m_dataResourceLoader;
    ppc::crypto::CryptoBox::Ptr m_cryptoBox;
    ppc::crypto::Hash::Ptr m_binHashImpl;

    constexpr static unsigned PRIVATE_KEY_HEX_LEN = 64;
};
}  // namespace ppc::initializer