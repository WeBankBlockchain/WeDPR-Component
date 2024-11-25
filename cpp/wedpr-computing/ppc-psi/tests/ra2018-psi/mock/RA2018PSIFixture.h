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
 * @file RA2018PSIFixture.h
 * @author: yujiechen
 * @date 2022-11-6
 */
#pragma once
#include "FakeRA2018PSIStorage.h"
#include "ppc-crypto-core/src/hash/MD5Hash.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-crypto/src/oprf/RA2018Oprf.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "test-utils/FakeFront.h"
#include "test-utils/FakePPCMessage.h"
#include <ppc-psi/src/ra2018-psi/RA2018PSIFactory.h>

using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::psi;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::front;
using namespace ppc::tools;

namespace ppc::test
{
class FakeRA2018Impl : public RA2018PSIImpl
{
public:
    using Ptr = std::shared_ptr<FakeRA2018Impl>;
    FakeRA2018Impl(RA2018PSIConfig::Ptr _config, RA2018PSIStorage::Ptr _storage,
        unsigned _idleTimeMs = 0, bool _waitResult = false, bool _disabled = false)
      : RA2018PSIImpl(_config, _storage, _idleTimeMs, _waitResult, _disabled)
    {
        // set the m_started flag to be true
        m_started = true;
        m_taskSyncTimer->registerTimeoutHandler([this]() { syncTaskInfo(); });
        m_taskSyncTimer->start();
    }

    ~FakeRA2018Impl() override = default;
};

class FakeRA2018PSIFactory : public RA2018PSIFactory
{
public:
    using Ptr = std::shared_ptr<FakeRA2018PSIFactory>;
    FakeRA2018PSIFactory()
      : m_front(std::make_shared<FakeFront>()),
        m_ppcMsgFactory(std::make_shared<FakePPCMessageFactory>()),
        m_dataResourceLoader(std::make_shared<DataResourceLoaderImpl>(
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)),
        m_threadPool(std::make_shared<ThreadPool>("ra2018-psi", 4))
    {
        auto hashImpl = std::make_shared<Sha256Hash>();
        auto eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::P256);
        m_cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(hashImpl, eccCrypto);
        m_binHashImpl = std::make_shared<MD5Hash>();
    }

    ~FakeRA2018PSIFactory() override = default;

    RA2018PSIImpl::Ptr createRA2018PSI(std::string const& _selfParty,
        ppc::front::FrontInterface::Ptr _front, ppc::tools::PPCConfig::Ptr const& _config,
        ppc::crypto::RA2018OprfInterface::Ptr const& _oprf, ppc::crypto::Hash::Ptr _binHashImpl,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory,
        ppc::storage::SQLStorage::Ptr _storage, ppc::storage::FileStorage::Ptr _fileStorage,
        bcos::ThreadPool::Ptr _threadPool, ppc::io::DataResourceLoader::Ptr _dataResourceLoader,
        uint32_t minNeededMemoryGB = 1) override
    {
        auto const& ra2018Config = _config->ra2018PSIConfig();
        auto config = std::make_shared<RA2018PSIConfig>(_selfParty, _front, _oprf, _binHashImpl,
            _ppcMsgFactory, ra2018Config.cuckooFilterOption, _threadPool, _storage, _fileStorage,
            _dataResourceLoader, 10000, minNeededMemoryGB, ra2018Config.dbName,
            ra2018Config.cuckooFilterCacheSize, ra2018Config.cacheSize, ra2018Config.dataBatchSize);
        // enforce the taskExpireTime to 3000ms
        config->setTaskExpireTime(3000);
        // use the FakeRA2018PSIStorage
        auto PSIStorage = std::make_shared<FakeRA2018PSIStorage>(config);
        // wait-for the task completed or exceptioned to call the notification-callback
        return std::make_shared<FakeRA2018Impl>(config, PSIStorage, 0, true, false);
    }

    RA2018PSIImpl::Ptr createRA2018PSI(std::string const& _selfParty, PPCConfig::Ptr const& _config)
    {
        auto const& ra2018Config = _config->ra2018PSIConfig();
        auto oprf = std::make_shared<RA2018Oprf>(
            _config->privateKey(), m_cryptoBox->eccCrypto(), m_cryptoBox->hashImpl());
        return createRA2018PSI(_selfParty, m_front, _config, oprf, m_binHashImpl, m_ppcMsgFactory,
            nullptr, nullptr, m_threadPool, m_dataResourceLoader);
    }

    DataResourceLoaderImpl::Ptr resourceLoader() { return m_dataResourceLoader; }
    FakeFront::Ptr front() { return m_front; }
    CryptoBox::Ptr cryptoBox() { return m_cryptoBox; }

private:
    FakeFront::Ptr m_front;
    CryptoBox::Ptr m_cryptoBox;
    Hash::Ptr m_binHashImpl;
    FakePPCMessageFactory::Ptr m_ppcMsgFactory;
    DataResourceLoaderImpl::Ptr m_dataResourceLoader;
    ThreadPool::Ptr m_threadPool;
};
}  // namespace ppc::test