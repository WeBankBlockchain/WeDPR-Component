/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file EcdhPSIFixutre.h
 * @author: yujiechen
 * @date 2022-12-29
 */
#pragma once
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto/src/ecc/ECDHCryptoFactoryImpl.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "test-utils/FakeFront.h"
#include "test-utils/FakePPCMessage.h"
#include <ppc-psi/src/ecdh-psi/EcdhPSIFactory.h>
#include <ppc-psi/src/ecdh-psi/EcdhPSIMessageFactory.h>


using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::psi;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::front;
using namespace ppc::tools;

namespace ppc::test
{
class FakeEcdhPSIImpl : public EcdhPSIImpl
{
public:
    using Ptr = std::shared_ptr<FakeEcdhPSIImpl>;
    FakeEcdhPSIImpl(EcdhPSIConfig::Ptr const& _config, unsigned _idleTimeMs = 0)
      : EcdhPSIImpl(_config, _idleTimeMs)
    {
        // set the m_started flag to be true
        m_started = true;
        m_enableOutputExists = true;
        m_taskSyncTimer->registerTimeoutHandler([this]() { syncTaskInfo(); });
        m_taskSyncTimer->start();
    }
    ~FakeEcdhPSIImpl() override = default;
};

class FakeEcdhPSIFactory : public EcdhPSIFactory
{
public:
    using Ptr = std::shared_ptr<FakeEcdhPSIFactory>;
    FakeEcdhPSIFactory()
      : m_front(std::make_shared<FakeFront>()),
        m_ppcMsgFactory(std::make_shared<FakePPCMessageFactory>()),
        m_dataResourceLoader(std::make_shared<DataResourceLoaderImpl>(
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)),
        m_threadPool(std::make_shared<ThreadPool>("ra2018-psi", 4))
    {
        auto hashImpl = std::make_shared<Sha256Hash>();
        auto eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::P256);
        m_cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(hashImpl, eccCrypto);
    }

    ~FakeEcdhPSIFactory() override = default;

    EcdhPSIImpl::Ptr createEcdhPSI(ppc::tools::PPCConfig::Ptr const& _ppcConfig,
        ppc::crypto::ECDHCryptoFactory::Ptr const& _ecdhCryptoFactory,
        ppc::front::FrontInterface::Ptr _front,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory, bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr const& _dataResourceLoader,
        uint32_t minNeededMemoryGB = 1) override
    {
        auto psiMsgFactory = std::make_shared<EcdhPSIMessageFactory>();
        auto const& ecdhParam = _ppcConfig->ecdhPSIConfig();
        auto config = std::make_shared<EcdhPSIConfig>(_ppcConfig->agencyID(), _ecdhCryptoFactory,
            _front, _ppcMsgFactory, psiMsgFactory, _dataResourceLoader, ecdhParam.dataBatchSize,
            10000, minNeededMemoryGB, _threadPool);
        // enforce the taskExpireTime to 3000ms
        config->setTaskExpireTime(3000);
        return std::make_shared<FakeEcdhPSIImpl>(config);
    }

    EcdhPSIImpl::Ptr createEcdhPSI(std::string const& _selfParty, PPCConfig::Ptr const& _config)
    {
        _config->setAgencyID(_selfParty);
        auto const& ra2018Config = _config->ra2018PSIConfig();
        auto ecdhCryptoFactory = std::make_shared<ECDHCryptoFactoryImpl>(_config->privateKey());
        return createEcdhPSI(
            _config, ecdhCryptoFactory, m_front, m_ppcMsgFactory, nullptr, m_dataResourceLoader);
    }
    DataResourceLoaderImpl::Ptr resourceLoader() { return m_dataResourceLoader; }
    FakeFront::Ptr front() { return m_front; }
    CryptoBox::Ptr cryptoBox() { return m_cryptoBox; }

private:
    FakeFront::Ptr m_front;
    CryptoBox::Ptr m_cryptoBox;
    FakePPCMessageFactory::Ptr m_ppcMsgFactory;
    DataResourceLoaderImpl::Ptr m_dataResourceLoader;
    ThreadPool::Ptr m_threadPool;
};
}  // namespace ppc::test