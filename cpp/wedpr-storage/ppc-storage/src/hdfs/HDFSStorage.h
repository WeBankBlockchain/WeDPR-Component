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
 * @file HDFSStorage.h
 * @author: yujiechen
 * @date 2022-11-30
 */
#pragma once
#include "Common.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-framework/storage/FileStorage.h"
#include <bcos-utilities/Log.h>
#include <hdfs/hdfs.h>
#include <sstream>
#include <string>

namespace ppc::storage
{
//// the hdfs handler
class HDFSHandler : public FileHandler
{
public:
    using Ptr = std::shared_ptr<HDFSHandler>;
    HDFSHandler(std::string const& _path,
        std::shared_ptr<HdfsFileSystemInternalWrapper> _storageHandler,
        hdfsFileInfo* _fileInfoHandler)
      : m_path(_path), m_storageHandler(_storageHandler), m_fileInfoHandler(_fileInfoHandler)
    {}
    ~HDFSHandler() override { closeFile(); }

    void* fileInfoHandler() const override { return (void*)m_fileInfoHandler.get(); }
    void* storageHandler() const override { return (void*)m_storageHandler.get(); }
    std::string const& path() const override { return m_path; }

    void closeFile() { m_fileInfoHandler.reset(); }

private:
    std::string m_path;
    std::shared_ptr<HdfsFileSystemInternalWrapper> m_storageHandler;

    struct HDFSFileInfoDeleter
    {
        void operator()(hdfsFileInfo* _fileInfo) const
        {
            if (!_fileInfo)
            {
                return;
            }
            hdfsFreeFileInfo(_fileInfo, 1);
        }
    };
    std::unique_ptr<hdfsFileInfo, HDFSFileInfoDeleter> m_fileInfoHandler;
};

class HDFSStorage : public FileStorage
{
public:
    using Ptr = std::shared_ptr<HDFSStorage>;
    HDFSStorage(ppc::protocol::FileStorageConnectionOption::Ptr const& _connectOption);
    ~HDFSStorage() {}

    void createDirectory(std::string const& _dirPath) const override;
    FileHandler::Ptr openFile(
        std::string const& _path, bool _createIfNotExists = false) const override;
    // close the file
    void closeFile(FileHandler::Ptr _handler) const override
    {
        auto hdfsHandler = std::dynamic_pointer_cast<HDFSHandler>(_handler);
        hdfsHandler->closeFile();
    }

    void renameFile(std::string const& _old_path, std::string const& _new_path) const override;

    void deleteFile(std::string const& _file) const override;

    bool fileExists(std::string const& _path) const override;

private:
    inline std::string getHomeDirectory() const
    {
        return std::string("/user/") + m_option->userName;
    }

    void createDirectoryImpl(std::string const& _dirPath) const;

    void tryToCreateHomeDirectory() const;

private:
    ppc::protocol::FileStorageConnectionOption::Ptr m_option;
    struct HDFSBuilderDeleter
    {
        void operator()(hdfsBuilder* _builder) const { hdfsFreeBuilder(_builder); }
    };
    std::unique_ptr<hdfsBuilder, HDFSBuilderDeleter> m_builder;

    struct HDFSFSDeleter
    {
        void operator()(HdfsFileSystemInternalWrapper* _fs) const { hdfsDisconnect(_fs); }
    };
    std::shared_ptr<HdfsFileSystemInternalWrapper> m_fs;
};
}  // namespace ppc::storage