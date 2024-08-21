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
 * @file LabeledPSIConfig.h
 * @author: shawnhe
 * @date 2022-11-3
 */

#pragma once

#include "Common.h"
#include "ppc-crypto/src/oprf/EcdhOprf.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/crypto/Oprf.h"
#include "ppc-protocol/src/PPCMessage.h"
#include "ppc-psi/src/PSIConfig.h"

#include <apsi/item.h>
#include <bcos-utilities/ThreadPool.h>

namespace ppc::psi
{
class LabeledPSIConfig : public PSIConfig
{
public:
    using Ptr = std::shared_ptr<LabeledPSIConfig>;

    LabeledPSIConfig(std::string const& _selfPartyID, ppc::front::FrontInterface::Ptr _front,
        ppc::crypto::CryptoBox::Ptr _cryptoBox, bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader, int _holdingMessageMinutes,
        front::PPCMessageFactory::Ptr _msgFactory = std::make_shared<front::PPCMessageFactory>())
      : PSIConfig(ppc::protocol::PSIAlgorithmType::LABELED_PSI_2PC, _selfPartyID, std::move(_front),
            _msgFactory, _dataResourceLoader, _holdingMessageMinutes),
        m_cryptoBox(std::move(_cryptoBox)),
        m_threadPool(std::move(_threadPool)),
        m_messageFactory(_msgFactory)
    {
        m_oprfServer = std::make_shared<crypto::EcdhOprfServer>(
            sizeof(apsi::Item::value_type) + sizeof(apsi::LabelKey), m_cryptoBox->hashImpl(),
            m_cryptoBox->eccCrypto());
    }

    virtual ~LabeledPSIConfig() = default;

public:
    crypto::Hash::Ptr const& hash() const { return m_cryptoBox->hashImpl(); }
    crypto::EccCrypto::Ptr const& eccCrypto() const { return m_cryptoBox->eccCrypto(); }
    bcos::ThreadPool::Ptr const& threadPool() const { return m_threadPool; }
    crypto::OprfServer::Ptr const& oprfServer() const { return m_oprfServer; }

private:
    ppc::crypto::CryptoBox::Ptr m_cryptoBox;
    bcos::ThreadPool::Ptr m_threadPool;

    crypto::OprfServer::Ptr m_oprfServer;
    front::PPCMessageFactory::Ptr m_messageFactory;
};

}  // namespace ppc::psi