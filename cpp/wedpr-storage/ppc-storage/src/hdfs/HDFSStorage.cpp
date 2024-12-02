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
 * @file HDFSStorage.cpp
 * @author: yujiechen
 * @date 2022-11-30
 */
#include "HDFSStorage.h"
#include "auth/Krb5CredLoader.h"
#include <hdfs/hdfs.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

using namespace ppc::storage;
using namespace ppc::protocol;
using namespace bcos;

HDFSStorage::HDFSStorage(FileStorageConnectionOption::Ptr const& _option)
  : m_option(_option), m_builder(hdfsNewBuilder())
{
    if (!m_option)
    {
        BOOST_THROW_EXCEPTION(HDFSConnectionOptionNotSet()
                              << errinfo_comment("Must set the hdfs-connection-option!"));
    }
    // Note: the libhdfs3 will try to connect to the hdfs with connect-timeout and retry
    // 'rpc.client.connect.retry' times
    // the default connect timeout is 600s
    // the default 'rpc.client.connect.retry' is 10
    auto connectTimeout =
        std::to_string(_option->connectionTimeout);  // set 1s as the connectTimeout
    HDFS_STORAGE_LOG(INFO) << LOG_DESC("create HDFSStorage") << _option->desc()
                           << LOG_KV("connectTimeout", connectTimeout);
    hdfsBuilderConfSetStr(m_builder.get(), "rpc.client.connect.timeout", connectTimeout.c_str());

    // default disable output.replace-datanode-on-failure, set to false to resolve append data error
    if (_option->replaceDataNodeOnFailure)
    {
        hdfsBuilderConfSetStr(m_builder.get(), "output.replace-datanode-on-failure", "true");
    }
    else
    {
        hdfsBuilderConfSetStr(m_builder.get(), "output.replace-datanode-on-failure", "false");
    }

    // set name node
    hdfsBuilderSetNameNode(m_builder.get(), _option->nameNode.c_str());
    hdfsBuilderSetNameNodePort(m_builder.get(), _option->nameNodePort);
    if (!_option->userName.empty())
    {
        hdfsBuilderSetUserName(m_builder.get(), _option->userName.c_str());
    }
    if (!_option->token.empty())
    {
        hdfsBuilderSetToken(m_builder.get(), _option->token.c_str());
    }
    // init the auth information
    if (_option->authConfig)
    {
        // set auth type to Kerberos
        hdfsBuilderConfSetStr(m_builder.get(), "hadoop.security.authentication", "kerberos");
        // init and store the auth information into the cache
        auto ctx = std::make_shared<Krb5Context>(_option->authConfig);
        ctx->init();
        HDFS_STORAGE_LOG(INFO) << LOG_DESC("SetKerbTicketCachePath")
                               << LOG_KV("ccachePath", _option->authConfig->ccachePath);
        // set the ccache file path
        hdfsBuilderSetKerbTicketCachePath(m_builder.get(), _option->authConfig->ccachePath.c_str());
    }
    // connect to the hdfs, Note: the m_fs is a pointer
    m_fs = std::shared_ptr<HdfsFileSystemInternalWrapper>(
        hdfsBuilderConnect(m_builder.get()), HDFSFSDeleter());
    if (!m_fs)
    {
        BOOST_THROW_EXCEPTION(
            ConnectToHDFSFailed() << errinfo_comment(
                "Connect to hdfs failed! error: " + std::string(hdfsGetLastError())));
    }
    HDFS_STORAGE_LOG(INFO) << LOG_DESC("create HDFSStorage success") << _option->desc()
                           << LOG_KV("connectTimeout", connectTimeout);
}


void HDFSStorage::tryToCreateHomeDirectory() const
{
    auto homeDir = getHomeDirectory();
    if (hdfsExists(m_fs.get(), homeDir.c_str()) != 0)
    {
        createDirectoryImpl(homeDir);
    }
}

void HDFSStorage::createDirectory(std::string const& _dirPath) const
{
    tryToCreateHomeDirectory();
    createDirectoryImpl(_dirPath);
}

void HDFSStorage::createDirectoryImpl(std::string const& _dirPath) const
{
    HDFS_STORAGE_LOG(INFO) << LOG_DESC("createDirectory: ") << _dirPath;
    if (hdfsCreateDirectory(m_fs.get(), _dirPath.c_str()) != 0)
    {
        BOOST_THROW_EXCEPTION(CreateDirectoryFailed() << bcos::errinfo_comment(
                                  "createDirectory " + _dirPath +
                                  " failed, error: " + std::string(hdfsGetLastError())));
    }
    HDFS_STORAGE_LOG(INFO) << LOG_DESC("createDirectory success") << LOG_KV("path", _dirPath);
}

FileHandler::Ptr HDFSStorage::openFile(std::string const& _path, bool _createIfNotExists) const
{
    // check the existence of the file
    if (hdfsExists(m_fs.get(), _path.c_str()) != 0)
    {
        if (!_createIfNotExists)
        {
            BOOST_THROW_EXCEPTION(OpenFileFailed() << bcos::errinfo_comment(
                                      "OpenFileFailed: The file " + _path + " not found!"));
        }
        else
        {
            HDFS_STORAGE_LOG(DEBUG)
                << LOG_DESC("openFile: create the non-exists file") << LOG_KV("path", _path);
            boost::filesystem::path filePath(_path);
            auto parentPath = filePath.parent_path().string();
            if (parentPath.empty())
            {
                parentPath = getHomeDirectory();
            }
            // create the parent path
            if (hdfsExists(m_fs.get(), parentPath.c_str()) != 0)
            {
                HDFS_STORAGE_LOG(DEBUG) << LOG_DESC("create parent path: ") << parentPath;
                createDirectoryImpl(parentPath);
            }
            auto fd = hdfsOpenFile(m_fs.get(), _path.c_str(), O_CREAT, 0, 0, 0);
            if (!fd)
            {
                HDFS_STORAGE_LOG(WARNING)
                    << LOG_DESC("openFileFailed: create the non-exists file failed")
                    << LOG_KV("path", _path)
                    << LOG_KV("exception", std::string(hdfsGetLastError()));
                BOOST_THROW_EXCEPTION(
                    OpenFileFailed() << bcos::errinfo_comment(
                        "OpenFileFailed: create the non-exists file failed, path: " + _path +
                        ", error: " + std::string(hdfsGetLastError())));
            }
            // release the fd
            hdfsCloseFile(m_fs.get(), fd);
        }
    }
    auto fileInfo = hdfsGetPathInfo(m_fs.get(), _path.c_str());
    HDFS_STORAGE_LOG(DEBUG) << LOG_DESC("HDFSStorage: openFile success") << LOG_KV("path", _path)
                            << LOG_KV("blockSize", fileInfo->mBlockSize)
                            << LOG_KV("blockNum", fileInfo->mSize);

    return std::make_shared<HDFSHandler>(_path, m_fs, fileInfo);
}

// delete the file
void HDFSStorage::deleteFile(std::string const& _path) const
{
    HDFS_STORAGE_LOG(INFO) << LOG_DESC("deleteFile") << LOG_KV("path", _path);
    auto ret = hdfsDelete(m_fs.get(), _path.c_str(), 1);
    // delete success
    if (!ret)
    {
        return;
    }
    // delete failed
    BOOST_THROW_EXCEPTION(DeleteHDFSFileFailed()
                          << bcos::errinfo_comment("DeleteHDFSFileFailed, path: " + _path +
                                                   ", error: " + std::string(hdfsGetLastError())));
}

void HDFSStorage::renameFile(std::string const& _old_path, std::string const& _new_path) const
{
    HDFS_STORAGE_LOG(INFO) << LOG_DESC("renameFile") << LOG_KV("old_path", _old_path)
                           << LOG_KV("new_path", _new_path);
    auto ret = hdfsRename(m_fs.get(), _old_path.c_str(), _new_path.c_str());
    // delete success
    if (!ret)
    {
        return;
    }
    // delete failed
    BOOST_THROW_EXCEPTION(RenameHDFSFileFailed()
                          << bcos::errinfo_comment("RenameHDFSFileFailed, path: " + _old_path +
                                                   ", error: " + std::string(hdfsGetLastError())));
}

bool HDFSStorage::fileExists(std::string const& _path) const
{
    if (hdfsExists(m_fs.get(), _path.c_str()) != 0)
    {
        return false;
    }
    return true;
}