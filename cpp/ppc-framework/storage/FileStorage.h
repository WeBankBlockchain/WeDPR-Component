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
 * @file FileStorage.h
 * @author: yujiechen
 * @date 2022-11-30
 */
#pragma once
#include "../protocol/Protocol.h"
#include <memory>

namespace ppc::storage
{
class FileHandler
{
public:
    using Ptr = std::shared_ptr<FileHandler>;
    FileHandler() = default;
    virtual ~FileHandler() = default;

    virtual void* fileInfoHandler() const = 0;
    virtual void* storageHandler() const = 0;
    virtual std::string const& path() const = 0;
};

class FileStorage
{
public:
    using Ptr = std::shared_ptr<FileStorage>;
    FileStorage() = default;
    virtual ~FileStorage() = default;

    // create the directory
    virtual void createDirectory(std::string const& _dirPath) const = 0;
    // open the file
    virtual FileHandler::Ptr openFile(
        std::string const& _path, bool _createIfNotExists = false) const = 0;
    // close the file
    virtual void closeFile(FileHandler::Ptr _handler) const = 0;

    // delete the specified file
    virtual void deleteFile(std::string const& _path) const = 0;

    // check the specified file exists or not
    virtual bool fileExists(std::string const& _path) const = 0;

    virtual void renameFile(std::string const& _old_path, std::string const& _new_path) const = 0;
};

class FileStorageFactory
{
public:
    using Ptr = std::shared_ptr<FileStorageFactory>;
    FileStorageFactory() = default;
    virtual ~FileStorageFactory() = default;

    virtual FileStorage::Ptr createFileStorage(ppc::protocol::DataResourceType _type,
        ppc::protocol::FileStorageConnectionOption::Ptr const& _option) = 0;
};
}  // namespace ppc::storage