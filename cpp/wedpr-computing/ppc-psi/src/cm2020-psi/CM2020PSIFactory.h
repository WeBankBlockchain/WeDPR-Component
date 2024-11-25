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
 * @file CM2020PSIFactory.h
 * @author: shawnhe
 * @date 2022-12-7
 */
#pragma once
#include "CM2020PSIImpl.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/io/DataResourceLoader.h"

namespace ppc::psi
{
class CM2020PSIFactory
{
public:
    using Ptr = std::shared_ptr<CM2020PSIFactory>;
    CM2020PSIFactory() = default;
    virtual ~CM2020PSIFactory() = default;

    virtual CM2020PSIImpl::Ptr buildCM2020PSI(std::string const& _selfParty,
        ppc::front::FrontInterface::Ptr _front, ppc::crypto::CryptoBox::Ptr _cryptoBox,
        bcos::ThreadPool::Ptr _threadPool, ppc::io::DataResourceLoader::Ptr _dataResourceLoader,
        int _holdingMessageMinutes, uint16_t _parallelism, uint32_t minNeededMemoryGB = 1)
    {
        auto config = std::make_shared<CM2020PSIConfig>(_selfParty, std::move(_front),
            std::move(_cryptoBox), std::move(_threadPool), std::move(_dataResourceLoader),
            _holdingMessageMinutes, minNeededMemoryGB, _parallelism);
        return std::make_shared<CM2020PSIImpl>(config);
    }
};
}  // namespace ppc::psi