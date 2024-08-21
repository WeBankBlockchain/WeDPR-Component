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
 * @file RemoteStorage.h
 * @author: shawnhe
 * @date 2023-1-29
 */
#pragma once
#include "../protocol/Protocol.h"
#include <memory>

namespace ppc::storage
{

class RemoteStorage
{
public:
    using Ptr = std::shared_ptr<RemoteStorage>;
    RemoteStorage() = default;
    virtual ~RemoteStorage() = default;

    virtual protocol::FileInfo::Ptr upload(std::string const& _path) = 0;
    virtual void download(protocol::FileInfo::Ptr _fileInfo) = 0;
};

class RemoteStorageFactory
{
public:
    using Ptr = std::shared_ptr<RemoteStorageFactory>;
    RemoteStorageFactory() = default;
    virtual ~RemoteStorageFactory() = default;

    virtual RemoteStorage::Ptr createRemoteStorage(ppc::protocol::DataResourceType _type,
        ppc::protocol::RemoteStorageConnectionOption::Ptr const& _option) = 0;
};
}  // namespace ppc::storage