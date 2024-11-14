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
 * @file FileStorageFactoryImpl.h
 * @author: yujiechen
 * @date 2022-11-30
 */
#pragma once
#include "hdfs/HDFSStorage.h"

namespace ppc::storage
{
class FileStorageFactoryImpl : public FileStorageFactory
{
public:
    using Ptr = std::shared_ptr<FileStorageFactoryImpl>;
    FileStorageFactoryImpl() = default;
    ~FileStorageFactoryImpl() override = default;

    FileStorage::Ptr createFileStorage(ppc::protocol::DataResourceType _type,
        ppc::protocol::FileStorageConnectionOption::Ptr const& _option) override
    {
        switch (_type)
        {
        case ppc::protocol::DataResourceType::HDFS:
        {
            _option->check();
            return std::make_shared<HDFSStorage>(_option);
        }
        default:
        {
            BOOST_THROW_EXCEPTION(
                UnsupportedFileStorage() << bcos::errinfo_comment("Only support HDFS now"));
        }
        }
    }
};
}  // namespace ppc::storage