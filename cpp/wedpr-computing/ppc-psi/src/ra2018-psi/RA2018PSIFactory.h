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
 * @file RA2018PSIFactory.h
 * @author: yujiechen
 * @date 2022-11-14
 */
#pragma once
#include "RA2018PSIImpl.h"
#include "storage/RA2018PSIStorage.h"
namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::psi
{
class RA2018PSIFactory
{
public:
    using Ptr = std::shared_ptr<RA2018PSIFactory>;
    RA2018PSIFactory() = default;
    virtual ~RA2018PSIFactory() = default;

    virtual RA2018PSIImpl::Ptr createRA2018PSI(std::string const& _selfParty,
        ppc::front::FrontInterface::Ptr _front,
        std::shared_ptr<ppc::tools::PPCConfig> const& _config,
        ppc::crypto::RA2018OprfInterface::Ptr const& _oprf, ppc::crypto::Hash::Ptr _binHashImpl,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory,
        ppc::storage::SQLStorage::Ptr _storage, ppc::storage::FileStorage::Ptr _fileStorage,
        bcos::ThreadPool::Ptr _threadPool, ppc::io::DataResourceLoader::Ptr _dataResourceLoader,
        uint32_t minNeededMemoryGB = 1);
};
}  // namespace ppc::psi