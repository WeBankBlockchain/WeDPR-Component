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
 * @file EcdhCache.h
 * @author: yujiechen
 * @date 2022-12-28
 */
#pragma once
#include "../psi-framework/TaskState.h"
#include "Common.h"
#include "EcdhPSIConfig.h"
#include <bcos-utilities/Common.h>
#include <memory>
namespace ppc::psi
{
/// the server data-cache
class ServerCipherDataCache
{
public:
    using Ptr = std::shared_ptr<ServerCipherDataCache>;
    ServerCipherDataCache(std::string const& _taskID) : m_taskID(_taskID) {}
    virtual ~ServerCipherDataCache()
    {
        // release the memory
        std::set<bcos::bytes>().swap(m_serverCipherData);
    }

    void setServerDataBatchCount(int64_t _serverDataBatchCount)
    {
        m_serverDataBatchCount = _serverDataBatchCount;
        if ((int64_t)m_serverDataSeqList.size() == m_serverDataBatchCount)
        {
            m_serverDataLoadFinished = true;
        }
    }

    virtual void appendServerCipherData(uint32_t _seq, std::vector<bcos::bytes>&& _cipherData)
    {
        bcos::WriteGuard l(x_serverCipherData);
        if (m_serverDataSeqList.count(_seq))
        {
            return;
        }
        for (auto& it : _cipherData)
        {
            m_serverCipherData.insert(std::move(it));
        }
        m_serverDataSeqList.insert(_seq);
        if ((int64_t)m_serverDataSeqList.size() == m_serverDataBatchCount)
        {
            m_serverDataLoadFinished = true;
        }
    }

    bool serverDataLoadFinished() { return m_serverDataLoadFinished; }
    std::set<bcos::bytes> const& serverCipherData() const { return m_serverCipherData; }

private:
    std::string m_taskID;
    bool m_serverDataLoadFinished = false;

    // store the cipher-data of the server
    // TODO: replace with unordered_set
    std::set<bcos::bytes> m_serverCipherData;
    std::set<uint32_t> m_serverDataSeqList;
    bcos::SharedMutex x_serverCipherData;
    std::atomic<int64_t> m_serverDataBatchCount = {-1};
};

class DataCache : public std::enable_shared_from_this<DataCache>
{
public:
    using Ptr = std::shared_ptr<DataCache>;
    // set the plainData here for obtain the plain intersection
    DataCache(EcdhPSIConfig::Ptr const& _config, std::string const& _taskID, uint32_t _seq,
        TaskState::Ptr const& _taskState, ServerCipherDataCache::Ptr const& _serverCipherData,
        ppc::io::DataBatch::Ptr&& _plainData)
      : m_config(_config),
        m_taskID(_taskID),
        m_seq(_seq),
        m_taskState(_taskState),
        m_state(CacheState::Evaluating),
        m_serverCipherData(_serverCipherData),
        m_plainData(std::move(_plainData))
    {
        m_capacity = m_plainData->capacityBytes();
        // for m_clientCipherData
        m_capacity += m_plainData->size() * 32;
    }
    virtual ~DataCache()
    {
        // release the memory
        m_plainData = nullptr;
        std::vector<bcos::bytes>().swap(m_clientCipherData);
        std::vector<bcos::bytes>().swap(m_intersectionData);
    }
    virtual void setClientCipherData(std::vector<bcos::bytes>&& _cipherData);
    virtual bool tryToIntersectionAndStoreResult();
    std::string const& taskID() const { return m_taskID; }

    CacheState state() const { return m_state; }

    uint64_t capacity() const { return m_capacity; }

    inline std::string printCurrentState()
    {
        std::ostringstream stringstream;
        stringstream << LOG_KV("task", m_taskID) << LOG_KV("seq", m_seq)
                     << LOG_KV("plainData", m_plainData ? m_plainData->size() : 0)
                     << LOG_KV("clientCipherData", m_clientCipherData.size())
                     << LOG_KV("intersectionData", m_intersectionData.size())
                     << LOG_KV("state", m_state);
        return stringstream.str();
    }

private:
    void storePSIResult();
    void syncPSIResult();

private:
    EcdhPSIConfig::Ptr m_config;
    std::string m_taskID;
    // the sub-task seq
    uint32_t m_seq;
    TaskState::Ptr m_taskState;

    CacheState m_state;

    uint64_t m_capacity;

    // the server cipher data cache
    ServerCipherDataCache::Ptr m_serverCipherData;

    // the plainData
    ppc::io::DataBatch::Ptr m_plainData;
    // the cipher-data of the client
    std::vector<bcos::bytes> m_clientCipherData;
    bcos::SharedMutex x_clientCipherData;

    // the intersection data
    std::vector<bcos::bytes> m_intersectionData;

    mutable bcos::RecursiveMutex m_mutex;
};

class EcdhCache : public std::enable_shared_from_this<EcdhCache>
{
public:
    using Ptr = std::shared_ptr<EcdhCache>;
    EcdhCache(EcdhPSIConfig::Ptr const& _config, uint64_t _maxCacheSize = 1024 * 1024 * 1024)
      : m_config(_config), m_maxTaskCacheSize(_maxCacheSize)
    {}
    virtual ~EcdhCache() = default;

    // insert the server-cipher-data-cache
    virtual ServerCipherDataCache::Ptr insertServerCipherCache(
        std::string const& _taskID, TaskState::Ptr const& _taskState);
    // get the server cipher data cache
    virtual ServerCipherDataCache::Ptr getServerCipherDataCache(std::string const& _taskID);

    // insert data-cache into m_taskCache
    virtual DataCache::Ptr insertSubTaskCache(std::string const& _taskID, uint32_t _seq,
        TaskState::Ptr const& _taskState, ServerCipherDataCache::Ptr const& _serverCipherData,
        ppc::io::DataBatch::Ptr&& _plainData);

    DataCache::Ptr getSubTaskCache(std::string const& _taskID, uint32_t _seq) const;
    // Note: only when the task-finished can erase the cache
    virtual void eraseCache(std::string const& _taskID);
    virtual void tryToIntersectionAndStoreResult();

    bool isFull() { return m_capacity >= m_maxTaskCacheSize; }

    uint64_t capacity() const { return m_capacity; }

private:
    void eraseTaskCache(std::string const& _taskID);

private:
    EcdhPSIConfig::Ptr m_config;

    uint64_t m_maxTaskCacheSize;
    // the capacity for sub-task
    // Note: the intersection-data and cipherDataStore may occupy some memory
    std::atomic<uint64_t> m_capacity = {0};

    // taskID => seq => sub-task-data
    std::map<std::string, std::map<uint32_t, DataCache::Ptr>> m_taskCache;
    mutable bcos::SharedMutex x_taskCache;

    // taskID => the server cipher data
    std::map<std::string, ServerCipherDataCache::Ptr> m_serverDataStore;
    mutable bcos::SharedMutex x_serverDataStore;
};
}  // namespace ppc::psi