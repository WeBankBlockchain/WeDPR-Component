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
 * @file RA2018PSIStorage.cpp
 * @author: yujiechen
 * @date 2022-10-27
 */
#include "RA2018PSIStorage.h"
#include "../Common.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <boost/lexical_cast.hpp>
#include <sstream>

using namespace ppc;
using namespace bcos;
using namespace ppc::psi;
using namespace ppc::storage;
using namespace ppc::protocol;
using namespace ppc::io;

// create the database
void RA2018PSIStorage::createDataBase()
{
    std::string command = "CREATE DATABASE IF NOT EXISTS " + m_config->dbName();
    m_config->storage()->execCommit(command.c_str(), (int)FieldDataType::TERMINATE);
    RA2018_LOG(INFO) << LOG_DESC("RA2018PSIStorage: create database success")
                     << LOG_KV("dbName", m_config->dbName());
    if (m_config->fileStorage())
    {
        m_config->fileStorage()->createDirectory(m_config->dbName());
    }
    RA2018_LOG(INFO) << LOG_DESC("RA2018PSIStorage: create directory success")
                     << LOG_KV("dirPath", m_config->dbName());
}

// create the cuckoo-filter-table
// TODO: add hash field to the cuckooFilterTable
void RA2018PSIStorage::createCuckooFilterTable()
{
    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS `" << m_config->cuckooFilterTableName() << "` (\n";
    ss << "`" << m_resourceIDField << "` varchar(128),\n";
    ss << "`" << m_filterIDField << "` INT AUTO_INCREMENT,\n";
    // Note: here filter-hash-field use at-most 32Bytes, for perf-consideration, default use md5
    ss << "`" << m_filterHashField << "` TINYBLOB DEFAULT NULL,\n";
    // Note: the mysql will convert boolean to tinyint
    ss << "`" << m_loadField << "` BOOLEAN,\n";
    ss << " PRIMARY KEY (`" << m_resourceIDField << "`, `" << m_filterIDField << "`)\n";
    ss << ", key(" << m_filterIDField << ")";
    ss << ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";
    m_config->storage()->execCommit(ss.str().c_str(), FieldDataType::TERMINATE);
    RA2018_LOG(INFO) << LOG_DESC("createCuckooFilterTable: create cuckoo-filter-table success")
                     << LOG_KV("dbName", m_config->cuckooFilterTableName());
}

// get the cuckoo-filter-ids according to the _resourceID
CuckooFilterInfoSet RA2018PSIStorage::getCuckooFilterInfos(
    std::string const& _resourceID, bool _onlyNotFull)
{
    try
    {
        auto startT = utcSteadyTime();
        std::stringstream ss;
        ss << "select `" << m_filterIDField << "`, `" << m_filterHashField << "` from `"
           << m_config->cuckooFilterTableName() << "` where `" << m_resourceIDField << "`='"
           << _resourceID << "'";
        if (_onlyNotFull)
        {
            ss << " and `" << m_loadField << "` = 0";
        }
        auto sqlCommand = ss.str();
        auto result =
            m_config->storage()->execQuery(true, sqlCommand.c_str(), FieldDataType::TERMINATE);
        if (result->size() != 2)
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PPCRetCode::EXCEPTION,
                "Invalid queried data, expect 2 columns, the result is " +
                    std::to_string(result->size()) + " columns"));
        }
        // parse the result
        RA2018_LOG(INFO) << LOG_DESC("getCuckooFilterInfos success")
                         << LOG_KV("metaFieldSize", result->metaData().size());
        // parse the filter data
        // int64_t type
        auto const& filterIDData = result->data().at(0);
        // bytes type
        auto const& filterHashData = result->data().at(1);
        if (filterIDData->size() != filterHashData->size())
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PPCRetCode::EXCEPTION,
                "Invalid queried data, the item size of filterIDs should be equal to the size of "
                "filterHashData"));
        }

        CuckooFilterInfoSet filterInfos;
        for (uint64_t i = 0; i < filterIDData->size(); i++)
        {
            auto filterID = filterIDData->get<int64_t>(i);
            auto hash = filterHashData->get<bcos::bytes>(i);
            filterInfos.insert(std::make_shared<CuckooFilterInfo>(filterID, hash, nullptr));
            RA2018_LOG(INFO) << LOG_DESC("getCuckooFilterInfos") << LOG_KV("resource", _resourceID)
                             << LOG_KV("filterID", filterID) << LOG_KV("hash", toHex(hash));
        }
        RA2018_LOG(INFO) << LOG_DESC("getCuckooFilterInfos success")
                         << LOG_KV("resource", _resourceID)
                         << LOG_KV("filterSize", filterInfos.size())
                         << LOG_KV("timecost", (utcSteadyTime() - startT));
        return filterInfos;
    }
    catch (std::exception const& e)
    {
        std::stringstream oss;
        oss << "getCuckooFilterInfos error! " << LOG_KV("resource", _resourceID)
            << LOG_KV("error", boost::diagnostic_information(e));
        auto errorMsg = oss.str();
        RA2018_LOG(WARNING) << errorMsg;
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PPCRetCode::EXCEPTION, errorMsg));
    }
}

void RA2018PSIStorage::storeFilterData(std::string const& _desc, std::string const& _resourceID,
    bcos::bytes const& _hash, bytesConstRef _data)
{
    auto startT = utcSteadyTime();
    auto hexHash = toHex(_hash);
    auto desc = obtainCuckooFilterDesc(_resourceID, hexHash);
    auto writer = m_config->dataResourceLoader()->loadWriter(desc, true, m_config->fileStorage());
    writer->writeBytes(_data);
    writer->flush();
    RA2018_LOG(INFO) << LOG_BADGE("storeFilterData success") << LOG_DESC(_desc)
                     << LOG_KV("timecost", (utcSteadyTime() - startT))
                     << LOG_KV("resource", _resourceID) << LOG_KV("hash", hexHash)
                     << LOG_KV("dataSize", _data.size()) << LOG_KV("path", desc->path());
}

// remove the old filter when updating cuckoo-filter
void RA2018PSIStorage::removeOldFilterData(
    std::string const& _desc, std::string const& _resourceID, bcos::bytes const& _hash)
{
    auto hexHash = toHex(_hash);
    auto desc = obtainCuckooFilterDesc(_resourceID, hexHash);
    try
    {
        if (desc->type() == (int)ppc::protocol::DataResourceType::FILE)
        {
            std::remove(desc->path().c_str());
        }
        if (desc->type() == (int)ppc::protocol::DataResourceType::HDFS)
        {
            m_config->fileStorage()->deleteFile(desc->path());
        }
    }
    catch (std::exception const& e)
    {
        RA2018_LOG(WARNING) << LOG_DESC("removeOldFilterData error")
                            << LOG_KV("resource", _resourceID) << LOG_KV("hash", hexHash)
                            << LOG_KV("path", desc->path());
    }
}

// store the cuckoo-filter to the storage
void RA2018PSIStorage::asyncInsertCuckooFilter(std::string const& _resourceID,
    DefaultCukooFilterPtr _cukooFilter, std::function<void(bcos::Error::Ptr&&)> _callback,
    int32_t _filterID)
{
    std::stringstream ss;
    auto filterData = _cukooFilter->serialize();
    auto filterDataHash = m_config->binHashImpl()->hash(
        bytesConstRef((const bcos::byte*)(filterData.data()), filterData.size()));
    storeFilterData("asyncInsertCuckooFilter", _resourceID, filterDataHash,
        bytesConstRef((bcos::byte const*)filterData.data(), filterData.size()));

    RA2018_LOG(INFO) << LOG_DESC("asyncInsertCuckooFilter: update meta-info")
                     << LOG_KV("resource", _resourceID) << LOG_KV("dataSize", filterData.size());

    auto cuckooFilterFull = _cukooFilter->full() ? 1 : 0;
    auto withFilterID = (_filterID == -1) ? false : true;

    ss << "insert into " << m_config->cuckooFilterTableName() << "(";
    ss << m_resourceIDField << ", " << m_loadField << ", " << m_filterHashField;
    if (withFilterID)
    {
        ss << "," << m_filterIDField;
    }
    // resourceID, loadField, filterHash
    ss << ") values(?, ?, ?";
    // filterID
    if (withFilterID)
    {
        ss << ",?";
    }
    // support update when key-conflict
    ss << ")ON DUPLICATE KEY UPDATE `" << m_resourceIDField << "`=?, `" << m_loadField << "`=?, `"
       << m_filterHashField << "`=?";
    if (withFilterID)
    {
        ss << ", `" << m_filterIDField << "` = ?";
    }
    // the command
    auto sqlCommand = ss.str();
    // resourceID, loadField, filterDataHash
    auto statement =
        m_config->storage()->generateStatement(sqlCommand.c_str(), FieldDataType::STRING,
            _resourceID.c_str(), FieldDataType::UINT, cuckooFilterFull, FieldDataType::BYTES,
            filterDataHash.data(), filterDataHash.size(), FieldDataType::TERMINATE);
    // filterID
    if (withFilterID)
    {
        m_config->storage()->appendStatement(
            statement, FieldDataType::SINT, _filterID, FieldDataType::TERMINATE);
    }
    m_config->storage()->appendStatement(statement, FieldDataType::STRING, _resourceID.c_str(),
        FieldDataType::UINT, cuckooFilterFull, FieldDataType::BYTES, filterDataHash.data(),
        filterDataHash.size(), FieldDataType::TERMINATE);
    if (withFilterID)
    {
        m_config->storage()->appendStatement(
            statement, FieldDataType::SINT, _filterID, FieldDataType::TERMINATE);
    }
    asyncCommit("asyncInsertCuckooFilter", _resourceID, statement, _callback);
}

void RA2018PSIStorage::asyncUpdateCuckooFilter(std::string const& _resourceID,
    CuckooFilterInfo::Ptr const& _filterInfo, std::function<void(bcos::Error::Ptr&&)> _callback)
{
    if (!_filterInfo->cuckooFilter())
    {
        _callback(BCOS_ERROR_PTR(-1, "asyncUpdateCuckooFilter failed for empty cuckoo-filter"));
        return;
    }
    // the hash not changed
    auto filterData = _filterInfo->cuckooFilter()->serialize();
    auto filterDataHash = m_config->binHashImpl()->hash(
        bytesConstRef((const bcos::byte*)(filterData.data()), filterData.size()));

    if (filterDataHash == _filterInfo->hash())
    {
        RA2018_LOG(INFO) << LOG_DESC("asyncUpdateCuckooFilter return for the filter not changed")
                         << _filterInfo->desc();
        _callback(nullptr);
        return;
    }
    // remove the old-cuckoo-filter
    removeOldFilterData("asyncUpdateCuckooFilter", _resourceID, _filterInfo->hash());
    // store the updated-cuckoo-filter
    storeFilterData("asyncUpdateCuckooFilter", _resourceID, filterDataHash,
        bytesConstRef((bcos::byte const*)filterData.data(), filterData.size()));

    RA2018_LOG(INFO) << LOG_DESC("asyncUpdateCuckooFilter: updateMetaInfo")
                     << LOG_KV("resource", _resourceID) << _filterInfo->desc()
                     << LOG_KV("currentHash", bcos::toHex(filterDataHash))
                     << LOG_KV("dataSize", filterData.size());
    std::stringstream ss;
    auto cuckooFilterFull = _filterInfo->cuckooFilter()->full() ? 1 : 0;

    ss << "update `" << m_config->cuckooFilterTableName() << "` set ";
    ss << m_loadField << " = ?, ";
    ss << m_filterHashField << " = ?";
    ss << " where " << m_resourceIDField << " = '" << _resourceID << "'";
    ss << " and " << m_filterIDField << " = " << _filterInfo->filterID();
    auto sqlCommand = ss.str();
    auto statement = m_config->storage()->generateStatement(sqlCommand.c_str(), FieldDataType::UINT,
        cuckooFilterFull, FieldDataType::BYTES, filterDataHash.data(), filterDataHash.size(),
        FieldDataType::TERMINATE);
    asyncCommit("asyncUpdateCuckooFilter", _resourceID, statement, _callback);
}

void RA2018PSIStorage::asyncCommit(std::string&& _desc, std::string const& _resourceID,
    ppc::storage::Statement::Ptr _statement, std::function<void(bcos::Error::Ptr&&)> _callback)
{
    auto self = weak_from_this();
    m_config->threadPool()->enqueue(
        [_statement, desc = std::move(_desc), _resourceID, _callback, self]() {
            try
            {
                auto storage = self.lock();
                if (!storage)
                {
                    return;
                }
                storage->m_config->storage()->execStatement(_statement);
                _callback(nullptr);
            }
            catch (std::exception const& e)
            {
                std::stringstream oss;
                oss << desc << " error !" << LOG_KV("resource", _resourceID)
                    << LOG_KV("error", boost::diagnostic_information(e));
                auto errorMsg = oss.str();
                RA2018_LOG(WARNING) << errorMsg;
                _callback(BCOS_ERROR_PTR((int64_t)PPCRetCode::EXCEPTION, errorMsg));
            }
        });
}

CuckooFilterInfo::Ptr RA2018PSIStorage::loadCuckooFilterFromFile(std::string const& _resourceID,
    int32_t _filterID, bcos::bytes const& _cuckooFilterHash, bool _deserialize)
{
    try
    {
        auto startT = utcSteadyTime();
        auto cuckooFilterInfo = std::make_shared<CuckooFilterInfo>(_filterID, _cuckooFilterHash);
        auto hexHash = toHex(_cuckooFilterHash);
        auto desc = obtainCuckooFilterDesc(_resourceID, hexHash);
        auto reader = m_config->dataResourceLoader()->loadReader(
            desc, DataSchema::Bytes, false, m_config->fileStorage());
        auto cuckooFilterData = reader->readBytes();
        auto dataSize = cuckooFilterData.size();
        // deserialize
        if (_deserialize)
        {
            RA2018_LOG(INFO) << LOG_DESC("deserialize cuckoo-filter") << cuckooFilterInfo->desc()
                             << LOG_KV("size", cuckooFilterData.size());
            // return the deserialized cuckoo-filter
            auto cuckooFilter = std::make_shared<DefaultCukooFilter>(
                bcos::bytesConstRef(cuckooFilterData.data(), cuckooFilterData.size()));
            cuckooFilterInfo->setCuckooFilter(std::move(cuckooFilter));
            RA2018_LOG(INFO) << LOG_DESC("deserialize cuckoo-filter success")
                             << cuckooFilterInfo->desc() << LOG_KV("size", cuckooFilterData.size());
        }
        else
        {
            // return the raw data
            cuckooFilterInfo->setCuckooFilterData(std::move(cuckooFilterData));
        }
        RA2018_LOG(INFO) << LOG_DESC("loadCuckooFilterFromFile success")
                         << LOG_KV("timecost", (utcSteadyTime() - startT))
                         << LOG_KV("resource", _resourceID) << cuckooFilterInfo->desc()
                         << LOG_KV("dataSize", dataSize) << LOG_KV("path", desc->path());
        return cuckooFilterInfo;
    }
    catch (OpenFileFailed const& e)
    {
        RA2018_LOG(WARNING) << LOG_DESC(
                                   "loadCuckooFilterFromFile exception for OpenFileFailed: delete "
                                   "the meta-info from sql-storage")
                            << LOG_KV("resource", _resourceID) << LOG_KV("filter", _filterID)
                            << LOG_KV("error", boost::diagnostic_information(e));
        // delete the meta-info from the storage
        deleteCuckooFilter(_resourceID, _filterID);
        throw e;
    }
}
// load the cuckoo-filter from the storage
CuckooFilterInfo::Ptr RA2018PSIStorage::loadCuckooFilterDataInfo(
    std::string const& _resourceID, int32_t _filterID, bool _deserialize)
{
    try
    {
        auto startT = utcSteadyTime();
        std::stringstream ss;
        ss << "select `" << m_filterHashField << "` "
           << " from `" << m_config->cuckooFilterTableName() << "`";
        ss << " where `" << m_resourceIDField << "` = '" << _resourceID << "'";
        ss << " and `" << m_filterIDField << "` = " << _filterID;
        auto command = ss.str();

        auto result =
            m_config->storage()->execQuery(true, command.c_str(), FieldDataType::TERMINATE);
        if (result->data().empty())
        {
            return nullptr;
        }
        if (result->size() != 1)
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PPCRetCode::EXCEPTION,
                "loadCuckooFilterDataInfo: invalid queried data, expect get 1 columns, real get " +
                    std::to_string(result->size()) + " columns!"));
        }
        // Note: only expect obtain one element from the database
        auto cuckooFilterHash = result->data().at(0)->get<bcos::bytes>(0);
        auto cuckooFilterInfo =
            loadCuckooFilterFromFile(_resourceID, _filterID, cuckooFilterHash, _deserialize);
        RA2018_LOG(INFO) << LOG_DESC("loadCuckooFilterDataInfo success")
                         << LOG_KV("timecost", (utcSteadyTime() - startT))
                         << LOG_KV("resource", _resourceID) << cuckooFilterInfo->desc();
        return cuckooFilterInfo;
    }
    catch (std::exception const& e)
    {
        std::stringstream oss;
        oss << "loadCuckooFilterDataInfo error! " << LOG_KV("resource", _resourceID)
            << LOG_KV("filter", _filterID) << LOG_KV("error", boost::diagnostic_information(e));
        auto errorMsg = oss.str();
        RA2018_LOG(WARNING) << errorMsg;
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PPCRetCode::EXCEPTION, errorMsg));
    }
}

void RA2018PSIStorage::deleteCuckooFilter(std::string const& _resourceID, int32_t _filterID)
{
    try
    {
        std::stringstream oss;
        oss << "delete from `" << m_config->cuckooFilterTableName() << "`";
        oss << " where `" << m_resourceIDField << "` = '" << _resourceID << "'";
        oss << " and `" << m_filterIDField << "` = " << _filterID;
        auto command = oss.str();
        m_config->storage()->execCommit(command.c_str(), FieldDataType::TERMINATE);
    }
    catch (std::exception const& e)
    {
        std::stringstream oss;
        oss << "deleteCuckooFilter error! " << LOG_KV("resource", _resourceID)
            << LOG_KV("filter", _filterID) << LOG_KV("error", boost::diagnostic_information(e));
        auto errorMsg = oss.str();
        RA2018_LOG(WARNING) << errorMsg;
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PPCRetCode::EXCEPTION, errorMsg));
    }
}