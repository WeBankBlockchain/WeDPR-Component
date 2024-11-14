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
 * @file DataResourceLoaderImpl.h
 * @author: yujiechen
 * @date 2022-11-4
 */
#pragma once
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/storage/FileStorage.h"
#include "ppc-framework/storage/RemoteStorage.h"
#include "ppc-framework/storage/SQLStorage.h"
#include <string>

namespace ppc::io
{
class DataResourceLoaderImpl : public DataResourceLoader
{
public:
    using Ptr = std::shared_ptr<DataResourceLoaderImpl>;
    DataResourceLoaderImpl(ppc::protocol::SQLConnectionOption::Ptr const& _sqlConnectionOpt,
        ppc::protocol::FileStorageConnectionOption::Ptr const& _fileStorageConnectionOpt,
        ppc::protocol::RemoteStorageConnectionOption::Ptr const& _remoteStorageConnectionOpt,
        ppc::storage::SQLStorageFactory::Ptr const& _sqlStorageFactory,
        ppc::storage::FileStorageFactory::Ptr const& _fileStorageFactory,
        ppc::storage::RemoteStorageFactory::Ptr const& _remoteStorageFactory);

    DataResourceLoaderImpl() = default;
    ~DataResourceLoaderImpl() override = default;

    LineReader::Ptr loadReader(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::io::DataSchema _schema = ppc::io::DataSchema::String, bool _parseByColumn = true,
        ppc::storage::FileStorage::Ptr const& _storage = nullptr) override;

    LineWriter::Ptr loadWriter(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        bool _truncate = false, ppc::storage::FileStorage::Ptr const& _storage = nullptr) override;

    void checkResourceExists(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::storage::FileStorage::Ptr const& _storage = nullptr) override;

    void deleteResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::storage::FileStorage::Ptr const& _storage = nullptr);

    void renameResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        std::string const& _new_path, ppc::storage::FileStorage::Ptr const& _storage = nullptr);

protected:
    virtual LineReader::Ptr loadSQLResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::io::DataSchema _schema, bool _parseByColumn);

    virtual LineReader::Ptr loadHDFSResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::storage::FileStorage::Ptr const& _fileStorage);

    void lazyLoadHdfsStorage();
    void lazyLoadSqlStorage();

private:
    // the sql storage
    ppc::protocol::SQLConnectionOption::Ptr m_sqlConnectionOpt;
    ppc::storage::SQLStorage::Ptr m_sqlStorage;
    bcos::Mutex x_sqlStorage;

    // the hdfs storage
    ppc::protocol::FileStorageConnectionOption::Ptr m_fileStorageConnectionOpt;
    ppc::storage::FileStorage::Ptr m_hdfsStorage;
    bcos::Mutex x_hdfsStorage;

    ppc::protocol::RemoteStorageConnectionOption::Ptr m_remoteStorageConnectionOpt;

    ppc::storage::SQLStorageFactory::Ptr m_sqlStorageFactory;
    ppc::storage::FileStorageFactory::Ptr m_fileStorageFactory;
    ppc::storage::RemoteStorageFactory::Ptr m_remoteStorageFactory;
};
}  // namespace ppc::io