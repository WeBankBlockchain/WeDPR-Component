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
 * @file EcdhConnPSIConfig.h
 * @author: zachma
 * @date 2023-7-17
 */
#pragma once
#include "../PSIConfig.h"
#include "EcdhConnPSIMessageFactory.h"
#include "ppc-framework/crypto/ECDHCrypto.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/ThreadPool.h>
#include <utility>

namespace ppc::psi
{
class EcdhConnPSIConfig : public PSIConfig
{
public:
    using Ptr = std::shared_ptr<EcdhConnPSIConfig>;
    EcdhConnPSIConfig(ppc::tools::PPCConfig::Ptr const& _ppcConfig,
        ppc::crypto::ECDHCryptoFactory::Ptr const& _ecdhCryptoFactory,
        ppc::front::FrontInterface::Ptr _front,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory,
        EcdhConnPSIMessageFactory::Ptr _psiMsgFactory,
        ppc::io::DataResourceLoader::Ptr const& _dataResourceLoader,
        bcos::ThreadPool::Ptr _threadPool)
      : PSIConfig(ppc::protocol::TaskAlgorithmType::ECDH_PSI_CONN, _ppcConfig->agencyID(), _front,
            _ppcMsgFactory, _dataResourceLoader, _ppcConfig->holdingMessageMinutes()),
        m_msgFactory(_psiMsgFactory),
        m_threadPool(_threadPool),
        m_ecdhCryptoFactory(_ecdhCryptoFactory),
        m_dataBatchSize(_ppcConfig->ecdhConnPSIConfig().dataBatchSize),
        m_config(_ppcConfig)
    {}

    virtual ~EcdhConnPSIConfig() = default;

    EcdhConnPSIMessageFactory::Ptr const& msgFactory() { return m_msgFactory; }
    ppc::crypto::ECDHCryptoFactory::Ptr const& ecdhCryptoFactory() const
    {
        return m_ecdhCryptoFactory;
    }

    ppc::tools::PPCConfig::Ptr const ppcConfig() { return m_config; }
    bcos::ThreadPool::Ptr const& threadPool() const { return m_threadPool; }

    uint32_t dataBatchSize() const { return m_dataBatchSize; }

private:
    EcdhConnPSIMessageFactory::Ptr m_msgFactory;
    bcos::ThreadPool::Ptr m_threadPool;
    ppc::crypto::ECDHCryptoFactory::Ptr m_ecdhCryptoFactory;
    ppc::tools::PPCConfig::Ptr m_config;

    uint32_t m_dataBatchSize = 10000;
};
}  // namespace ppc::psi