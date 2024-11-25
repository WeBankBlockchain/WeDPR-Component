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
 * @file RA2018PSIFactory.cpp
 * @author: yujiechen
 * @date 2022-11-14
 */
#include "RA2018PSIFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"

using namespace ppc::psi;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace ppc::storage;
using namespace ppc::tools;
using namespace ppc::io;

RA2018PSIImpl::Ptr RA2018PSIFactory::createRA2018PSI(std::string const& _selfParty,
    FrontInterface::Ptr _front, PPCConfig::Ptr const& _config,
    RA2018OprfInterface::Ptr const& _oprf, Hash::Ptr _binHashImpl,
    PPCMessageFaceFactory::Ptr _ppcMsgFactory, SQLStorage::Ptr _storage,
    FileStorage::Ptr _fileStorage, bcos::ThreadPool::Ptr _threadPool,
    DataResourceLoader::Ptr _dataResourceLoader, uint32_t minNeededMemoryGB)
{
    auto const& ra2018Config = _config->ra2018PSIConfig();

    auto config = std::make_shared<RA2018PSIConfig>(_selfParty, _front, _oprf, _binHashImpl,
        _ppcMsgFactory, ra2018Config.cuckooFilterOption, _threadPool, _storage, _fileStorage,
        _dataResourceLoader, _config->holdingMessageMinutes(), minNeededMemoryGB,
        ra2018Config.dbName, ra2018Config.cuckooFilterCacheSize, ra2018Config.cacheSize,
        ra2018Config.dataBatchSize);

    RA2018PSIStorage::Ptr psiStorage = nullptr;
    if (!_config->disableRA2018())
    {
        psiStorage = std::make_shared<RA2018PSIStorage>(config);
        // init the storage
        psiStorage->init();
    }
    return std::make_shared<RA2018PSIImpl>(config, psiStorage, 0, false, _config->disableRA2018());
}
