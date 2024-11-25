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
 * @file EcdhMultiPSIFactory.h
 */

#pragma once
#include "EcdhMultiPSIImpl.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/io/DataResourceLoader.h"

namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::psi
{
class EcdhMultiPSIFactory
{
public:
    using Ptr = std::shared_ptr<EcdhMultiPSIFactory>;
    EcdhMultiPSIFactory() = default;
    virtual ~EcdhMultiPSIFactory() = default;

    virtual EcdhMultiPSIImpl::Ptr createEcdhMultiPSI(
        std::shared_ptr<ppc::tools::PPCConfig> const& _ppcConfig,
        ppc::front::FrontInterface::Ptr _front, ppc::crypto::CryptoBox::Ptr _cryptoBox,
        bcos::ThreadPool::Ptr _threadPool, ppc::io::DataResourceLoader::Ptr _dataResourceLoader,
        uint32_t minNeededMemoryGB = 1);
};
}  // namespace ppc::psi