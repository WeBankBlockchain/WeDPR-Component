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
 * @file DataResourceLoaderImpl.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "DataResourceLoaderImpl.h"
#include "Common.h"
#include "FileLineReader.h"
#include "FileLineWriter.h"
#include "HDFSReader.h"
#include "HDFSWriter.h"
#include "SQLResultReader.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace ppc::io;
using namespace ppc::storage;
using namespace ppc::protocol;
using namespace bcos;


DataResourceLoaderImpl::DataResourceLoaderImpl(
    ppc::protocol::SQLConnectionOption::Ptr const& _sqlConnectionOpt,
    ppc::protocol::FileStorageConnectionOption::Ptr const& _fileStorageConnectionOpt,
    ppc::protocol::RemoteStorageConnectionOption::Ptr const& _remoteStorageConnectionOpt,
    ppc::storage::SQLStorageFactory::Ptr const& _sqlStorageFactory,
    ppc::storage::FileStorageFactory::Ptr const& _fileStorageFactory,
    ppc::storage::RemoteStorageFactory::Ptr const& _remoteStorageFactory)
  : m_sqlConnectionOpt(_sqlConnectionOpt),
    m_fileStorageConnectionOpt(_fileStorageConnectionOpt),
    m_remoteStorageConnectionOpt(_remoteStorageConnectionOpt),
    m_sqlStorageFactory(_sqlStorageFactory),
    m_fileStorageFactory(_fileStorageFactory),
    m_remoteStorageFactory(_remoteStorageFactory)
{}

void DataResourceLoaderImpl::lazyLoadHdfsStorage()
{
    if (!m_fileStorageConnectionOpt)
    {
        return;
    }
    bcos::Guard l(x_hdfsStorage);
    if (m_hdfsStorage)
    {
        return;
    }
    m_hdfsStorage =
        m_fileStorageFactory->createFileStorage(DataResourceType::HDFS, m_fileStorageConnectionOpt);
    IO_LOG(INFO) << LOG_DESC("lazyLoadHdfsStorage") << m_fileStorageConnectionOpt->desc();
    return;
}

void DataResourceLoaderImpl::lazyLoadSqlStorage()
{
    if (!m_sqlConnectionOpt)
    {
        return;
    }
    bcos::Guard l(x_sqlStorage);
    if (m_sqlStorage)
    {
        return;
    }
    m_sqlStorage =
        m_sqlStorageFactory->createSQLStorage(DataResourceType::MySQL, m_sqlConnectionOpt);
    IO_LOG(INFO) << LOG_DESC("lazyLoadSqlStorage") << m_sqlConnectionOpt->desc();
}

LineReader::Ptr DataResourceLoaderImpl::loadReader(DataResourceDesc::ConstPtr _desc,
    DataSchema _schema, bool _parseByColumn, FileStorage::Ptr const& _fileStorage)
{
    if (!_desc)
    {
        BOOST_THROW_EXCEPTION(LoadDataResourceException()
                              << bcos::errinfo_comment("must define the desc for data-resource"));
    }
    switch (_desc->type())
    {
        // load lineReader from file
    case (int)(DataResourceType::FILE):
    {
        return std::make_shared<FileLineReader>(_desc->path());
    }
    // load reader from the sql
    case (int)(DataResourceType::MySQL):
    {
        return loadSQLResource(_desc, _schema, _parseByColumn);
    }
    case (int)(DataResourceType::HDFS):
    {
        return loadHDFSResource(_desc, _fileStorage);
    }
    default:
    {
        BOOST_THROW_EXCEPTION(
            UnSupportedDataResource() << errinfo_comment("Only support File/MySQL/HDFS now"));
    }
    }
}

LineReader::Ptr DataResourceLoaderImpl::loadSQLResource(
    DataResourceDesc::ConstPtr _desc, DataSchema, bool _parseByColumn)
{
    IO_LOG(INFO) << LOG_DESC("loadSQLResource") << LOG_KV("command", _desc->accessCommand());
    SQLStorage::Ptr storage;
    if (_desc->sqlConnectionOption())
    {
        storage = m_sqlStorageFactory->createSQLStorage(
            (ppc::protocol::DataResourceType)_desc->type(), _desc->sqlConnectionOption());
    }
    else if (m_sqlConnectionOpt)
    {
        lazyLoadSqlStorage();
        storage = m_sqlStorage;
    }
    else
    {
        BOOST_THROW_EXCEPTION(
            ConnectionOptionNotFound() << errinfo_comment("mysql connection option not found"));
    }

    // exec command
    auto result = storage->execQuery(
        _parseByColumn, _desc->accessCommand().c_str(), FieldDataType::TERMINATE);
    return std::make_shared<SQLResultReader>(std::move(result), _parseByColumn);
}

LineReader::Ptr DataResourceLoaderImpl::loadHDFSResource(
    ppc::protocol::DataResourceDesc::ConstPtr _desc, FileStorage::Ptr const& _storage)
{
    IO_LOG(INFO) << LOG_DESC("loadHDFSResource") << LOG_KV("path", _desc->path());
    auto storage = _storage;
    if (!storage)
    {
        if (_desc->fileStorageConnectionOption())
        {
            storage = m_fileStorageFactory->createFileStorage(
                (ppc::protocol::DataResourceType)_desc->type(),
                _desc->fileStorageConnectionOption());
        }
        else if (m_fileStorageConnectionOpt)
        {
            lazyLoadHdfsStorage();
            storage = m_hdfsStorage;
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                ConnectionOptionNotFound() << errinfo_comment("hdfs connection option not found"));
        }
    }
    // load the HDFSReader, open the file
    auto handler = storage->openFile(_desc->path());
    return std::make_shared<HDFSReader>(handler);
}

void DataResourceLoaderImpl::deleteResource(
    ppc::protocol::DataResourceDesc::ConstPtr _desc, ppc::storage::FileStorage::Ptr const& _storage)
{
    if (!_desc)
    {
        BOOST_THROW_EXCEPTION(LoadDataResourceException()
                              << bcos::errinfo_comment("must define the desc for data-resource"));
    }
    switch (_desc->type())
    {
    case (int)(DataResourceType::FILE):
    case (int)(DataResourceType::HDFS):
    {
        auto storage = _storage;
        if (!storage)
        {
            if (_desc->fileStorageConnectionOption())
            {
                storage = m_fileStorageFactory->createFileStorage(
                    (ppc::protocol::DataResourceType)_desc->type(),
                    _desc->fileStorageConnectionOption());
            }
            else if (m_fileStorageConnectionOpt)
            {
                lazyLoadHdfsStorage();
                storage = m_hdfsStorage;
            }
            else
            {
                BOOST_THROW_EXCEPTION(ConnectionOptionNotFound()
                                      << errinfo_comment("hdfs connection option not found"));
            }
        }

        if (storage->fileExists(_desc->path()))
        {
            storage->deleteFile(_desc->path());
        }
        break;
    }
    default:
    {
        BOOST_THROW_EXCEPTION(
            UnSupportedDataResource() << errinfo_comment(
                "checkResourceExists: Only support File/HDFS now, passed in resource type: " +
                std::to_string(_desc->type())));
    }
    }
}

void DataResourceLoaderImpl::renameResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
    std::string const& _new_path, ppc::storage::FileStorage::Ptr const& _storage)
{
    if (!_desc)
    {
        BOOST_THROW_EXCEPTION(LoadDataResourceException()
                              << bcos::errinfo_comment("must define the desc for data-resource"));
    }
    switch (_desc->type())
    {
    case (int)(DataResourceType::FILE):
    case (int)(DataResourceType::HDFS):
    {
        auto storage = _storage;
        if (!storage)
        {
            if (_desc->fileStorageConnectionOption())
            {
                storage = m_fileStorageFactory->createFileStorage(
                    (ppc::protocol::DataResourceType)_desc->type(),
                    _desc->fileStorageConnectionOption());
            }
            else if (m_fileStorageConnectionOpt)
            {
                lazyLoadHdfsStorage();
                storage = m_hdfsStorage;
            }
            else
            {
                BOOST_THROW_EXCEPTION(ConnectionOptionNotFound()
                                      << errinfo_comment("hdfs connection option not found"));
            }
        }

        if (storage->fileExists(_new_path))
        {
            storage->deleteFile(_new_path);
        }
        storage->renameFile(_desc->path(), _new_path);
        break;
    }
    default:
    {
        BOOST_THROW_EXCEPTION(
            UnSupportedDataResource() << errinfo_comment(
                "checkResourceExists: Only support File/HDFS now, passed in resource type: " +
                std::to_string(_desc->type())));
    }
    }
}

void DataResourceLoaderImpl::checkResourceExists(
    ppc::protocol::DataResourceDesc::ConstPtr _desc, ppc::storage::FileStorage::Ptr const& _storage)
{
    if (!_desc)
    {
        BOOST_THROW_EXCEPTION(LoadDataResourceException()
                              << bcos::errinfo_comment("must define the desc for data-resource"));
    }
    switch (_desc->type())
    {
    case (int)(DataResourceType::FILE):
    {
        if (boost::filesystem::exists(boost::filesystem::path(_desc->path())))
        {
            BOOST_THROW_EXCEPTION(LoadDataResourceException() << bcos::errinfo_comment(
                                      "The file: " + _desc->path() + " already exists!"));
        }
        break;
    }
    case (int)(DataResourceType::HDFS):
    {
        auto storage = _storage;
        if (!storage)
        {
            if (_desc->fileStorageConnectionOption())
            {
                storage = m_fileStorageFactory->createFileStorage(
                    (ppc::protocol::DataResourceType)_desc->type(),
                    _desc->fileStorageConnectionOption());
            }
            else if (m_fileStorageConnectionOpt)
            {
                lazyLoadHdfsStorage();
                storage = m_hdfsStorage;
            }
            else
            {
                BOOST_THROW_EXCEPTION(ConnectionOptionNotFound()
                                      << errinfo_comment("hdfs connection option not found"));
            }
        }
        if (storage->fileExists(_desc->path()))
        {
            BOOST_THROW_EXCEPTION(LoadDataResourceException() << bcos::errinfo_comment(
                                      "The hdfs file: " + _desc->path() + " already exists!"));
        }
        break;
    }
    default:
    {
        BOOST_THROW_EXCEPTION(
            UnSupportedDataResource() << errinfo_comment(
                "checkResourceExists: Only support File/HDFS now, passed in resource type: " +
                std::to_string(_desc->type())));
    }
    }
}


LineWriter::Ptr DataResourceLoaderImpl::loadWriter(
    DataResourceDesc::ConstPtr _desc, bool _truncate, FileStorage::Ptr const& _storage)
{
    if (!_desc)
    {
        BOOST_THROW_EXCEPTION(LoadDataResourceException()
                              << bcos::errinfo_comment("must define the desc for data-resource"));
    }
    switch (_desc->type())
    {
    case (int)(DataResourceType::FILE):
    {
        return std::make_shared<FileLineWriter>(_desc->path(), _truncate);
    }
    case (int)(DataResourceType::HDFS):
    {
        auto storage = _storage;
        if (!storage)
        {
            if (_desc->fileStorageConnectionOption())
            {
                storage = m_fileStorageFactory->createFileStorage(
                    (ppc::protocol::DataResourceType)_desc->type(),
                    _desc->fileStorageConnectionOption());
            }
            else if (m_fileStorageConnectionOpt)
            {
                lazyLoadHdfsStorage();
                storage = m_hdfsStorage;
            }
            else
            {
                BOOST_THROW_EXCEPTION(ConnectionOptionNotFound()
                                      << errinfo_comment("hdfs connection option not found"));
            }
        }
        auto handler = storage->openFile(_desc->path(), true);
        return std::make_shared<HDFSWriter>(handler, _truncate);
    }
    default:
    {
        BOOST_THROW_EXCEPTION(
            UnSupportedDataResource() << errinfo_comment("Only support File/HDFS now"));
    }
    }
}