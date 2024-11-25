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
 * @file FakeLabeledPSIFactory.h
 * @author: shawnhe
 * @date 2022-12-26
 */
#pragma once

#include "ppc-crypto-core/src/hash/BLAKE2bHash.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "test-utils/FakeFront.h"
#include <ppc-psi/src/labeled-psi/LabeledPSIFactory.h>
using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::psi;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::front;
using namespace ppc::tools;

namespace ppc::test
{
class FakeLabeledPSIImpl : public LabeledPSIImpl
{
public:
    using Ptr = std::shared_ptr<FakeLabeledPSIImpl>;
    FakeLabeledPSIImpl(LabeledPSIConfig::Ptr const& _config, unsigned _idleTimeMs = 0)
      : LabeledPSIImpl(_config, _idleTimeMs)
    {}
    ~FakeLabeledPSIImpl() override = default;
};

class FakeLabeledPSIFactory : public LabeledPSIFactory
{
public:
    using Ptr = std::shared_ptr<FakeLabeledPSIFactory>;

    FakeLabeledPSIFactory()
      : m_front(std::make_shared<FakeFront>()),
        m_dataResourceLoader(std::make_shared<DataResourceLoaderImpl>(
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)),
        m_threadPool(std::make_shared<bcos::ThreadPool>("labeled-psi", 4))
    {
        auto hashImpl = std::make_shared<BLAKE2bHash>();
        auto eccCrypto = std::make_shared<Ed25519EccCrypto>();
        m_cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(hashImpl, eccCrypto);
    }

    ~FakeLabeledPSIFactory() override = default;

    LabeledPSIImpl::Ptr createLabeledPSI(std::string const& _selfParty)
    {
        auto config = std::make_shared<LabeledPSIConfig>(
            _selfParty, m_front, m_cryptoBox, m_threadPool, m_dataResourceLoader, 1);
        return std::make_shared<FakeLabeledPSIImpl>(config);
    }

    DataResourceLoaderImpl::Ptr resourceLoader() { return m_dataResourceLoader; }
    FakeFront::Ptr front() { return m_front; }
    CryptoBox::Ptr cryptoBox() { return m_cryptoBox; }

private:
    FakeFront::Ptr m_front;
    DataResourceLoaderImpl::Ptr m_dataResourceLoader;
    bcos::ThreadPool::Ptr m_threadPool;
    CryptoBox::Ptr m_cryptoBox;
};
}  // namespace ppc::test