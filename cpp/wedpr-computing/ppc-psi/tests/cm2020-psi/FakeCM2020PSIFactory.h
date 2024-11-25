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
 * @file FakeCM2020PSIFactory.h
 * @author: shawnhe
 * @date 2022-12-19
 */
#pragma once
#include "ppc-crypto-core/src/hash/BLAKE2bHash.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "test-utils/FakeFront.h"
#include <ppc-psi/src/cm2020-psi/CM2020PSIFactory.h>

using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::psi;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::front;
using namespace ppc::tools;

namespace ppc::test
{
class FakeCM2020PSIImpl : public CM2020PSIImpl
{
public:
    using Ptr = std::shared_ptr<FakeCM2020PSIImpl>;
    FakeCM2020PSIImpl(CM2020PSIConfig::Ptr const& _config, unsigned _idleTimeMs = 0)
      : CM2020PSIImpl(_config, _idleTimeMs)
    {}
    ~FakeCM2020PSIImpl() override = default;
};

class FakeCM2020PSIFactory : public CM2020PSIFactory
{
public:
    using Ptr = std::shared_ptr<FakeCM2020PSIFactory>;

    FakeCM2020PSIFactory()
      : m_front(std::make_shared<FakeFront>()),
        m_dataResourceLoader(std::make_shared<DataResourceLoaderImpl>(
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)),
        m_threadPool(std::make_shared<ThreadPool>("cm2018-psi", 4))
    {
        auto hashImpl = std::make_shared<BLAKE2bHash>();
        auto eccCrypto = std::make_shared<Ed25519EccCrypto>();
        m_cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(hashImpl, eccCrypto);
    }

    ~FakeCM2020PSIFactory() override = default;

    CM2020PSIImpl::Ptr createCM2020PSI(std::string const& _selfParty)
    {
        auto config = std::make_shared<CM2020PSIConfig>(
            _selfParty, m_front, m_cryptoBox, m_threadPool, m_dataResourceLoader, 1);

        return std::make_shared<FakeCM2020PSIImpl>(config);
    }

    DataResourceLoaderImpl::Ptr resourceLoader() { return m_dataResourceLoader; }
    FakeFront::Ptr front() { return m_front; }
    CryptoBox::Ptr cryptoBox() { return m_cryptoBox; }

private:
    FakeFront::Ptr m_front;
    DataResourceLoaderImpl::Ptr m_dataResourceLoader;
    ThreadPool::Ptr m_threadPool;
    CryptoBox::Ptr m_cryptoBox;
};
}  // namespace ppc::test