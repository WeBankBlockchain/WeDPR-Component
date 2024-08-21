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
 * @file CM2020PSIConfig.h
 * @author: shawnhe
 * @date 2022-12-7
 */

#pragma once
#include <bcos-utilities/ThreadPool.h>

#include <utility>

#include "Common.h"
#include "ppc-crypto/src/oprf/EcdhOprf.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/crypto/Oprf.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-protocol/src/PPCMessage.h"
#include "ppc-psi/src/PSIConfig.h"

using namespace ppc::psi;

namespace ppc::pir
{
class OtPIRConfig : public PSIConfig
{
public:
    using Ptr = std::shared_ptr<OtPIRConfig>;

    OtPIRConfig(std::string const& _selfPartyID, ppc::front::FrontInterface::Ptr _front,
        ppc::crypto::CryptoBox::Ptr _cryptoBox, bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader, int _holdingMessageMinutes,
        uint16_t _parallelism = 3,
        const front::PPCMessageFactory::Ptr& _msgFactory =
            std::make_shared<front::PPCMessageFactory>())
      : PSIConfig(ppc::protocol::PSIAlgorithmType::OT_PIR_2PC, _selfPartyID, std::move(_front),
            _msgFactory, std::move(_dataResourceLoader), _holdingMessageMinutes),
        m_cryptoBox(std::move(_cryptoBox)),
        m_threadPool(std::move(_threadPool)),
        m_parallelism(_parallelism)
    {}

    virtual ~OtPIRConfig() = default;

public:
    crypto::Hash::Ptr const& hash() const { return m_cryptoBox->hashImpl(); }
    crypto::EccCrypto::Ptr const& eccCrypto() const { return m_cryptoBox->eccCrypto(); }
    bcos::ThreadPool::Ptr const& threadPool() const { return m_threadPool; }
    uint16_t parallelism() const { return m_parallelism; }

private:
    ppc::crypto::CryptoBox::Ptr m_cryptoBox;
    bcos::ThreadPool::Ptr m_threadPool;
    uint16_t m_parallelism;
};

}  // namespace ppc::psi