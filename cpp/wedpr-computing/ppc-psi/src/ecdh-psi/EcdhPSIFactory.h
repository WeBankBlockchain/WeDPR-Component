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
 * @file EcdhPSIFactory.h
 * @author: yujiechen
 * @date 2022-12-28
 */
#pragma once
#include "EcdhPSIImpl.h"
#include <memory>
namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::psi
{
class EcdhPSIFactory
{
public:
    using Ptr = std::shared_ptr<EcdhPSIFactory>;
    EcdhPSIFactory() = default;
    virtual ~EcdhPSIFactory() = default;

    virtual EcdhPSIImpl::Ptr createEcdhPSI(std::shared_ptr<ppc::tools::PPCConfig> const& _ppcConfig,
        ppc::crypto::ECDHCryptoFactory::Ptr const& _ecdhCryptoFactory,
        ppc::front::FrontInterface::Ptr _front,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory, bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr const& _dataResourceLoader,
        uint32_t minNeededMemoryGB = 1);
};
}  // namespace ppc::psi