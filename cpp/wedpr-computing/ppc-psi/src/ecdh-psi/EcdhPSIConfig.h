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
 * @file EcdhPSIConfig.h
 * @author: yujiechen
 * @date 2022-12-26
 */
#pragma once
#include "../PSIConfig.h"
#include "../psi-framework/interfaces/PSIMessageFactory.h"
#include "ppc-framework/crypto/ECDHCrypto.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/ThreadPool.h>

namespace ppc::psi
{
class EcdhPSIConfig : public PSIConfig
{
public:
    using Ptr = std::shared_ptr<EcdhPSIConfig>;
    EcdhPSIConfig(std::string const& _selfParty,
        ppc::crypto::ECDHCryptoFactory::Ptr const& _ecdhCryptoFactory,
        ppc::front::FrontInterface::Ptr _front,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory,
        PSIMessageFactory::Ptr const& _msgFactory,
        ppc::io::DataResourceLoader::Ptr const& _dataResourceLoader, uint32_t _dataBatchSize,
        int _holdingMessageMinutes, uint32_t minNeededMemoryGB = 1,
        bcos::ThreadPool::Ptr const& _threadPool = nullptr,
        int _threadPoolSize = std::thread::hardware_concurrency())
      : PSIConfig(ppc::protocol::TaskAlgorithmType::ECDH_PSI_2PC, _selfParty, _front,
            _ppcMsgFactory, _dataResourceLoader, _holdingMessageMinutes, minNeededMemoryGB),
        m_msgFactory(_msgFactory),
        m_threadPool(_threadPool),
        m_ecdhCryptoFactory(_ecdhCryptoFactory),
        m_dataBatchSize(_dataBatchSize)
    {
        if (!m_threadPool)
        {
            m_threadPool = std::make_shared<bcos::ThreadPool>("ecdh-pool", _threadPoolSize);
        }
    }
    ~EcdhPSIConfig() override
    {
        if (m_threadPool)
        {
            m_threadPool->stop();
        }
    }

    PSIMessageFactory::Ptr const& msgFactory() const { return m_msgFactory; }
    bcos::ThreadPool::Ptr const& threadPool() const { return m_threadPool; }
    ppc::crypto::ECDHCryptoFactory::Ptr const& ecdhCryptoFactory() const
    {
        return m_ecdhCryptoFactory;
    }

    uint32_t dataBatchSize() const { return m_dataBatchSize; }

private:
    PSIMessageFactory::Ptr m_msgFactory;
    bcos::ThreadPool::Ptr m_threadPool;
    ppc::crypto::ECDHCryptoFactory::Ptr m_ecdhCryptoFactory;

    uint32_t m_dataBatchSize = 10000;
};
}  // namespace ppc::psi