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
 * @file RA2018PSIStorage.h
 * @author: yujiechen
 * @date 2022-10-27
 */

#pragma once
#include "../Common.h"
#include "../RA2018PSIConfig.h"
#include "../core/CuckooFilterInfo.h"
#include "ppc-framework/storage/SQLStorage.h"
#include <bcos-utilities/Error.h>
#include <memory>
#include <set>

namespace ppc::psi
{
class RA2018PSIStorage : public std::enable_shared_from_this<RA2018PSIStorage>
{
public:
    using Ptr = std::shared_ptr<RA2018PSIStorage>;
    RA2018PSIStorage(RA2018PSIConfig::Ptr _config) : m_config(_config) {}
    virtual ~RA2018PSIStorage() = default;

    // get the (cuckoo-filter-id, cuckoo-filter-hash according to the _resourceID and loadFactor
    // TODO: optimize the perf(use async-client)
    virtual CuckooFilterInfoSet getCuckooFilterInfos(
        std::string const& _resourceID, bool _onlyNotFull);

    // load the cuckoo-filter from the storage
    // TODO: optimize the perf(use async-client)
    virtual CuckooFilterInfo::Ptr loadCuckooFilterDataInfo(
        std::string const& _resourceID, int32_t _filterID, bool _deserialize);

    virtual bcos::bytes loadCuckooFilterData(std::string const& _resourceID, int32_t _filterID)
    {
        try
        {
            // need to deserialize
            auto filterInfo = loadCuckooFilterDataInfo(_resourceID, _filterID, false);
            if (!filterInfo)
            {
                return bcos::bytes();
            }
            return filterInfo->cuckooFilterData();
        }
        catch (std::exception const& e)
        {
            return bcos::bytes();
        }
    }

    virtual DefaultCukooFilterPtr loadCuckooFilter(
        std::string const& _resourceID, int32_t _filterID)
    {
        // need to serialize
        auto filterInfo = loadCuckooFilterDataInfo(_resourceID, _filterID, true);
        if (!filterInfo)
        {
            return nullptr;
        }
        return filterInfo->cuckooFilter();
    }

    // insert a new cuckoo-filter to the storage
    // update the entry if key-conflict
    virtual void asyncInsertCuckooFilter(std::string const& _resourceID,
        DefaultCukooFilterPtr _cukooFilter, std::function<void(bcos::Error::Ptr&&)> _callback,
        int32_t _filterID = -1);
    // update the cuckoo-filter
    // Note: update is more efficient than asyncInsertCuckooFilter
    virtual void asyncUpdateCuckooFilter(std::string const& _resourceID,
        CuckooFilterInfo::Ptr const& _filterInfo,
        std::function<void(bcos::Error::Ptr&&)> _callback);

    virtual void init()
    {
        RA2018_LOG(INFO) << LOG_DESC("RA2018PSIStorage init the database")
                         << LOG_KV("dbName", m_config->dbName());
        createDataBase();
        // use the created database
        RA2018_LOG(INFO) << LOG_DESC("RA2018PSIStorage use database: ") << m_config->dbName();
        m_config->storage()->useDataBase(m_config->dbName().c_str());

        RA2018_LOG(INFO) << LOG_DESC("RA2018PSIStorage init the cuckoo-filter-table")
                         << LOG_KV("dbName", m_config->cuckooFilterTableName());
        createCuckooFilterTable();
        RA2018_LOG(INFO) << LOG_DESC("RA2018PSIStorage init success");
    }

protected:
    // init the database for ra2018-storage
    void createDataBase();
    void createCuckooFilterTable();

    void asyncCommit(std::string&& _desc, std::string const& _resourceID,
        ppc::storage::Statement::Ptr _statement, std::function<void(bcos::Error::Ptr&&)> _callback);

    // store the filter data into file or hdfs
    void storeFilterData(std::string const& _desc, std::string const& _resourceID,
        bcos::bytes const& _hash, bcos::bytesConstRef _filterData);
    // remove the old filter when updating cuckoo-filter
    void removeOldFilterData(
        std::string const& _desc, std::string const& _resourceID, bcos::bytes const& _hash);

    CuckooFilterInfo::Ptr loadCuckooFilterFromFile(std::string const& _resourceID,
        int32_t _filterID, bcos::bytes const& _cuckooFilterHash, bool _deserialize);

    ppc::protocol::DataResourceDesc::Ptr obtainCuckooFilterDesc(
        std::string const& _resourceID, std::string const& _hexHash)
    {
        auto desc = std::make_shared<ppc::protocol::DataResourceDesc>();
        auto resourceIDHash = m_config->binHashImpl()->hash(
            bcos::bytesConstRef((bcos::byte const*)_resourceID.data(), _resourceID.size()));

        std::string path = m_config->dataPath() + "/" + bcos::toHex(resourceIDHash) + "/" +
                           _hexHash + c_dataPostFix;
        // use file
        if (!m_config->fileStorage())
        {
            desc->setType((uint16_t)ppc::protocol::DataResourceType::FILE);
        }
        else
        {
            // use hdfs
            // Note: we use dbName to distinguish files for different-nodes
            path = m_config->dbName() + "/" + path;
            desc->setType((uint16_t)ppc::protocol::DataResourceType::HDFS);
        }

        desc->setPath(path);
        return desc;
    }

    // delete the given cuckoo-filter
    virtual void deleteCuckooFilter(std::string const& _resourceID, int32_t _filterID);

private:
    RA2018PSIConfig::Ptr m_config;

    std::string const m_resourceIDField = "resource_id";
    // the filter-id auto-inc
    std::string const m_filterIDField = "filter_id";
    std::string const m_filterHashField = "hash";
    std::string const m_loadField = "full";

    std::string const c_dataPostFix = ".filter";
};
}  // namespace ppc::psi