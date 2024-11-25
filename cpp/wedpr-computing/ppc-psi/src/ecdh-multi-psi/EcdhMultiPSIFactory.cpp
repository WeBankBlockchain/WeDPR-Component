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
 * @file EcdhMultiPSIFactory.cpp
 */

#include "EcdhMultiPSIFactory.h"
#include "EcdhMultiPSIMessageFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"

using namespace ppc::psi;
using namespace ppc::front;
using namespace ppc::tools;
using namespace ppc::io;
using namespace ppc::crypto;

EcdhMultiPSIImpl::Ptr EcdhMultiPSIFactory::createEcdhMultiPSI(PPCConfig::Ptr const& _ppcConfig,
    FrontInterface::Ptr _front, CryptoBox::Ptr _cryptoBox, bcos::ThreadPool::Ptr _threadPool,
    DataResourceLoader::Ptr _dataResourceLoader, uint32_t minNeededMemoryGB)
{
    auto psiMsgFactory = std::make_shared<EcdhMultiPSIMessageFactory>();
    auto const& ecdhParam = _ppcConfig->ecdhMultiPSIConfig();
    auto _selfParty = _ppcConfig->agencyID();
    int _holdingMessageMinutes = _ppcConfig->holdingMessageMinutes();
    auto config = std::make_shared<EcdhMultiPSIConfig>(_selfParty, std::move(_front),
        std::move(_cryptoBox), std::move(_threadPool), std::move(_dataResourceLoader),
        ecdhParam.dataBatchSize, _holdingMessageMinutes, minNeededMemoryGB, psiMsgFactory);
    return std::make_shared<EcdhMultiPSIImpl>(config);
}
