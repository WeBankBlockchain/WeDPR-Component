/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file BsEcdhPSIFactory.h
 * @author: shawnhe
 * @date 2023-09-20
 */

#pragma once
#include "BsEcdhPSIFactory.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-psi/src/PSIConfig.h"
#include "ppc-psi/src/bs-ecdh-psi/BsEcdhPSIImpl.h"

namespace ppc::psi
{
class BsEcdhPSIFactory
{
public:
    using Ptr = std::shared_ptr<BsEcdhPSIFactory>;
    BsEcdhPSIFactory() = default;
    virtual ~BsEcdhPSIFactory() = default;

    virtual BsEcdhPSIImpl::Ptr buildBsEcdhPSI(bcos::ThreadPool::Ptr _threadPool,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader, uint32_t _timeoutMinutes,
        uint32_t minNeededMemoryGB = 1)
    {
        auto config = std::make_shared<PSIConfig>(ppc::protocol::TaskAlgorithmType::BS_ECDH_PSI,
            std::move(_dataResourceLoader), minNeededMemoryGB);
        return std::make_shared<BsEcdhPSIImpl>(
            std::move(config), std::move(_threadPool), _timeoutMinutes);
    }
};
}  // namespace ppc::psi