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
 * @file EcdhPSIFactory.cpp
 * @author: yujiechen
 * @date 2022-12-28
 */
#include "EcdhPSIFactory.h"
#include "EcdhPSIMessageFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <memory>

using namespace ppc::psi;
using namespace ppc::tools;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace ppc::io;

EcdhPSIImpl::Ptr EcdhPSIFactory::createEcdhPSI(PPCConfig::Ptr const& _ppcConfig,
    ECDHCryptoFactory::Ptr const& _ecdhCryptoFactory, FrontInterface::Ptr _front,
    PPCMessageFaceFactory::Ptr _ppcMsgFactory, bcos::ThreadPool::Ptr _threadPool,
    DataResourceLoader::Ptr const& _dataResourceLoader, uint32_t minNeededMemoryGB)
{
    auto psiMsgFactory = std::make_shared<EcdhPSIMessageFactory>();
    auto const& ecdhParam = _ppcConfig->ecdhPSIConfig();
    auto config = std::make_shared<EcdhPSIConfig>(_ppcConfig->agencyID(), _ecdhCryptoFactory,
        _front, _ppcMsgFactory, psiMsgFactory, _dataResourceLoader, ecdhParam.dataBatchSize,
        _ppcConfig->holdingMessageMinutes(), minNeededMemoryGB, _threadPool);
    return std::make_shared<EcdhPSIImpl>(config);
}
