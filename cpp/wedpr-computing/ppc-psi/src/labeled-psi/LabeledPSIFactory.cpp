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
 * @file LabeledPSIFactory.cpp
 * @author: shawnhe
 * @date 2022-11-14
 */
#include "LabeledPSIFactory.h"

using namespace ppc::psi;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::front;

LabeledPSIImpl::Ptr LabeledPSIFactory::buildLabeledPSI(std::string const& _selfParty,
    FrontInterface::Ptr _front, CryptoBox::Ptr _cryptoBox, bcos::ThreadPool::Ptr _threadPool,
    DataResourceLoader::Ptr _dataResourceLoader, int _holdingMessageMinutes,
    uint32_t minNeededMemoryGB)
{
    auto config = std::make_shared<LabeledPSIConfig>(_selfParty, std::move(_front),
        std::move(_cryptoBox), std::move(_threadPool), std::move(_dataResourceLoader),
        _holdingMessageMinutes, minNeededMemoryGB);
    return std::make_shared<LabeledPSIImpl>(config);
}