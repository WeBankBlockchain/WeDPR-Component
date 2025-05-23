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
 * @file EcdhConnPSIFactory.h
 * @author: zachma
 * @date 2023-7-18
 */

#pragma once
#include "EcdhConnPSIConfig.h"
#include "EcdhConnPSIImpl.h"
#include "EcdhConnPSIMessageFactory.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <memory>

namespace ppc::psi
{
class EcdhConnPSIFactory
{
public:
    using Ptr = std::shared_ptr<EcdhConnPSIFactory>;
    EcdhConnPSIFactory() = default;
    virtual ~EcdhConnPSIFactory() = default;

    virtual EcdhConnPSIImpl::Ptr createEcdhConnPSI(ppc::tools::PPCConfig::Ptr const& _ppcConfig,
        ppc::crypto::ECDHCryptoFactory::Ptr const& _ecdhCryptoFactory,
        ppc::front::FrontInterface::Ptr _front,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory, bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr const& _dataResourceLoader)
    {
        auto psiMsgFactory = std::make_shared<EcdhConnPSIMessageFactory>();
        auto config = std::make_shared<EcdhConnPSIConfig>(_ppcConfig, _ecdhCryptoFactory, _front,
            _ppcMsgFactory, psiMsgFactory, _dataResourceLoader, _threadPool);
        return std::make_shared<EcdhConnPSIImpl>(config);
    }
};
}  // namespace ppc::psi