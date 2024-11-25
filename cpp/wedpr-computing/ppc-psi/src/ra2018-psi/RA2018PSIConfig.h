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
 * @file RA2018PSIConfig.h
 * @author: yujiechen
 * @date 2022-10-25
 */
#pragma once
#include "../PSIConfig.h"
#include "Common.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/crypto/RA2018OprfInterface.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include "ppc-framework/storage/SQLStorage.h"
#include "protocol/RA2018Message.h"
#include <bcos-utilities/ThreadPool.h>

namespace ppc::psi
{
class RA2018PSIConfig : public PSIConfig
{
public:
    using Ptr = std::shared_ptr<RA2018PSIConfig>;
    RA2018PSIConfig(std::string const& _selfParty, ppc::front::FrontInterface::Ptr _front,
        ppc::crypto::RA2018OprfInterface::Ptr _oprf, ppc::crypto::Hash::Ptr _binHashImpl,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory,
        ppc::tools::CuckoofilterOption::Ptr _cuckooFilterOption, bcos::ThreadPool::Ptr _threadPool,
        ppc::storage::SQLStorage::Ptr _storage, ppc::storage::FileStorage::Ptr _fileStorage,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader, int _holdingMessageMinutes,
        uint32_t minNeededMemoryGB = 1, std::string const& _dbName = "ra2018",
        uint64_t _cuckooFilterCacheSize = 255 * 1024 * 1024,
        uint64_t _ra2018CacheCapacity = 1024 * 1024 * 1024, int _dataBatchSize = -1,
        int _threadPoolSize = std::thread::hardware_concurrency())
      : PSIConfig(ppc::protocol::TaskAlgorithmType::RA_PSI_2PC, _selfParty, _front, _ppcMsgFactory,
            _dataResourceLoader, _holdingMessageMinutes, minNeededMemoryGB),
        m_oprf(std::move(_oprf)),
        m_binHashImpl(std::move(_binHashImpl)),
        m_ra2018MsgFactory(std::make_shared<RA2018MessageFactory>()),
        m_threadPool(std::move(_threadPool)),
        m_storage(std::move(_storage)),
        m_fileStorage(std::move(_fileStorage)),
        m_dbName(_dbName),
        m_cuckooFilterOption(std::move(_cuckooFilterOption)),
        m_cuckooFilterCacheSize(_cuckooFilterCacheSize),
        m_ra2018CacheCapacity(_ra2018CacheCapacity),
        m_dataBatchSize(_dataBatchSize)
    {
        if (!m_threadPool)
        {
            m_threadPool = std::make_shared<bcos::ThreadPool>("ra2018", _threadPoolSize);
        }
    }
    ~RA2018PSIConfig() override = default;

    std::string const& dbName() const { return m_dbName; }
    std::string const& cuckooFilterTableName() const { return m_cuckooFilterTableName; }
    void setCukooFilterTableName(std::string const& _tableName)
    {
        m_cuckooFilterTableName = _tableName;
    }

    ppc::storage::SQLStorage::Ptr const& storage() { return m_storage; }
    ppc::storage::FileStorage::Ptr const& fileStorage() { return m_fileStorage; }

    bcos::ThreadPool::Ptr const& threadPool() { return m_threadPool; }

    ppc::io::DataResourceLoader::Ptr const& dataResourceLoader() { return m_dataResourceLoader; }
    ppc::crypto::RA2018OprfInterface::Ptr const& oprf() const { return m_oprf; }
    ppc::crypto::Hash::Ptr const& binHashImpl() const { return m_binHashImpl; }

    // the ra2018 message factory
    RA2018MessageFactory::Ptr const& ra2018MsgFactory() const { return m_ra2018MsgFactory; }
    ppc::tools::CuckoofilterOption::Ptr const& cuckooFilterOption() const
    {
        return m_cuckooFilterOption;
    }
    int64_t dataBatchSize() const { return m_dataBatchSize; }

    uint64_t cuckooFilterCacheSize() const { return m_cuckooFilterCacheSize; }
    uint64_t ra2018CacheCapacity() const { return m_ra2018CacheCapacity; }
    std::string const& dataPath() const { return m_dataPath; }

protected:
    ppc::crypto::RA2018OprfInterface::Ptr m_oprf;
    // the hash-impl used to calculate binary hash(for perf consideration, we default choose md5)
    ppc::crypto::Hash::Ptr m_binHashImpl;
    // the ra2018 message factory
    RA2018MessageFactory::Ptr m_ra2018MsgFactory;

    // the threadPool used to computation
    bcos::ThreadPool::Ptr m_threadPool;
    // the ppc-privacy-service storage
    ppc::storage::SQLStorage::Ptr m_storage;
    // the file-storage
    ppc::storage::FileStorage::Ptr m_fileStorage;

    // the db to store all data for RA2018
    std::string m_dbName = "ra2018";
    // the cuckoo-filter tableName, stores {data_resource-id, cuckoo-filter-id, cuckoo-filter-data}
    std::string m_cuckooFilterTableName = "t_cuckoo";

    // the cuckooFilterOption
    ppc::tools::CuckoofilterOption::Ptr m_cuckooFilterOption;

    uint64_t m_cuckooFilterCacheSize;
    uint64_t m_ra2018CacheCapacity = 0;
    // default load all data
    int64_t m_dataBatchSize = -1;

    std::string m_dataPath = "data";
};
}  // namespace ppc::psi