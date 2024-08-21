/*
 *  Copyright (C) 2023 WeDPR.
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
 * @file EcdhCache.cpp
 * @author: yujiechen
 * @date 2023-1-6
 */
#include "EcdhCache.h"
#include "gperftools/malloc_extension.h"

using namespace ppc::psi;
using namespace ppc::io;

void DataCache::setClientCipherData(std::vector<bcos::bytes>&& _cipherData)
{
    if (m_state != CacheState::Evaluating)
    {
        return;
    }
    bcos::WriteGuard l(x_clientCipherData);
    m_clientCipherData = std::move(_cipherData);
    m_state = CacheState::Finalized;
}

bool DataCache::tryToIntersectionAndStoreResult()
{
    try
    {
        bcos::RecursiveGuard l(m_mutex);
        auto startT = bcos::utcSteadyTime();
        if (m_state != CacheState::Finalized)
        {
            return false;
        }
        // the server data has not been load finished
        if (!m_serverCipherData->serverDataLoadFinished())
        {
            return false;
        }
        m_state = CacheState::IntersectionProgressing;
        auto const& serverCipherData = m_serverCipherData->serverCipherData();
        uint64_t offset = 0;
        for (auto const& data : m_clientCipherData)
        {
            if (serverCipherData.count(data))
            {
                m_intersectionData.emplace_back(m_plainData->getBytes(offset));
            }
            offset++;
        }
        m_state = CacheState::Intersectioned;
        ECDH_LOG(INFO) << LOG_DESC("tryToIntersectionAndStoreResult success")
                       << LOG_KV("intersections", m_intersectionData.size()) << printCurrentState()
                       << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
        // try to store psi-result
        storePSIResult();
        return true;
    }
    catch (std::exception const& e)
    {
        ECDH_LOG(WARNING) << LOG_DESC("tryToIntersectionAndStoreResult exception")
                          << printCurrentState() << LOG_KV("msg", boost::diagnostic_information(e));
        m_taskState->onTaskException(boost::diagnostic_information(e));
        return false;
    }
}

void DataCache::storePSIResult()
{
    if (m_state != CacheState::Intersectioned)
    {
        return;
    }
    m_state = CacheState::Stored;
    ECDH_LOG(INFO) << LOG_DESC("storePSIResult") << printCurrentState();
    m_taskState->storePSIResult(m_config->dataResourceLoader(), m_intersectionData);
    if (m_taskState->task()->syncResultToPeer())
    {
        syncPSIResult();
        return;
    }
    // update the taskState
    m_taskState->eraseFinishedTaskSeq(m_seq, true);
    m_state = CacheState::Synced;
}

void DataCache::syncPSIResult()
{
    // sync the psi-result to the psi-server by dataBatchSize
    auto self = weak_from_this();
    auto syncPSIResultMsg =
        m_config->msgFactory()->createPSIMessage((uint64_t)PSIPacketType::PSIResultSyncMsg);
    syncPSIResultMsg->setData(std::move(m_intersectionData));
    ECDH_LOG(INFO) << LOG_DESC("syncPSIResult") << printCurrentState();
    m_config->generateAndSendPPCMessage(
        m_taskState->peerID(), m_taskState->task()->id(), syncPSIResultMsg,
        [self](bcos::Error::Ptr&& _error) {
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            auto cache = self.lock();
            if (!cache)
            {
                return;
            }
            ECDH_LOG(WARNING) << LOG_DESC("syncPSIResult to peer error")
                              << LOG_KV("code", _error->errorCode())
                              << LOG_KV("msg", _error->errorMessage())
                              << printTaskInfo(cache->m_taskState->task());
        },
        m_seq,
        [self](bcos::Error::Ptr _error, std::string const& _agencyID,
            ppc::front::PPCMessageFace::Ptr _msg, ppc::front::ResponseFunc) {
            bool success = true;
            if (_error)
            {
                success = false;
                ECDH_LOG(WARNING) << LOG_DESC("syncPSIResult to peer error")
                                  << LOG_KV("code", _error->errorCode())
                                  << LOG_KV("msg", _error->errorMessage());
            }
            ECDH_LOG(INFO) << LOG_DESC("syncPSIResult: receive response")
                           << LOG_KV("seq", _msg->seq());
            auto cache = self.lock();
            if (!cache)
            {
                return;
            }
            cache->m_state = CacheState::Synced;
            cache->m_taskState->eraseFinishedTaskSeq(cache->m_seq, success);
        });
}

ServerCipherDataCache::Ptr EcdhCache::insertServerCipherCache(
    std::string const& _taskID, TaskState::Ptr const& _taskState)
{
    bcos::UpgradableGuard l(x_serverDataStore);
    auto it = m_serverDataStore.find(_taskID);
    if (it != m_serverDataStore.end())
    {
        return it->second;
    }
    bcos::UpgradeGuard ul(l);
    auto cache = std::make_shared<ServerCipherDataCache>(_taskID);
    m_serverDataStore[_taskID] = cache;
    return cache;
}


ServerCipherDataCache::Ptr EcdhCache::getServerCipherDataCache(std::string const& _taskID)
{
    bcos::ReadGuard l(x_serverDataStore);
    auto it = m_serverDataStore.find(_taskID);
    if (it != m_serverDataStore.end())
    {
        return it->second;
    }
    return nullptr;
}

DataCache::Ptr EcdhCache::insertSubTaskCache(std::string const& _taskID, uint32_t _seq,
    TaskState::Ptr const& _taskState, ServerCipherDataCache::Ptr const& _serverCipherData,
    ppc::io::DataBatch::Ptr&& _plainData)
{
    bcos::UpgradableGuard l(x_taskCache);
    if (!m_taskCache.count(_taskID))
    {
        std::map<uint32_t, DataCache::Ptr> subTaskCache;
        m_taskCache[_taskID] = subTaskCache;
    }
    auto it = m_taskCache.at(_taskID).find(_seq);
    if (it != m_taskCache.at(_taskID).end())
    {
        return it->second;
    }

    auto cache = std::make_shared<DataCache>(
        m_config, _taskID, _seq, _taskState, _serverCipherData, std::move(_plainData));
    bcos::UpgradeGuard ul(l);
    m_taskCache[_taskID][_seq] = cache;
    m_capacity += cache->capacity();
    return cache;
}

DataCache::Ptr EcdhCache::getSubTaskCache(std::string const& _taskID, uint32_t _seq) const
{
    bcos::ReadGuard l(x_taskCache);
    if (m_taskCache.count(_taskID) && m_taskCache.at(_taskID).count(_seq))
    {
        return m_taskCache.at(_taskID).at(_seq);
    }
    return nullptr;
}

void EcdhCache::tryToIntersectionAndStoreResult()
{
    bcos::WriteGuard l(x_taskCache);
    for (auto it = m_taskCache.begin(); it != m_taskCache.end();)
    {
        auto& clientCipherCache = it->second;
        for (auto pclientCipherCache = clientCipherCache.begin();
             pclientCipherCache != clientCipherCache.end();)
        {
            // Note: the cache must copy here in case of pclientCipherCache->second been erased
            auto cache = pclientCipherCache->second;
            // update the capacity and erase the finished sub-task
            if (cache->state() == CacheState::Synced)
            {
                pclientCipherCache = clientCipherCache.erase(pclientCipherCache);
                ECDH_LOG(INFO) << LOG_DESC(
                                      "tryToIntersectionAndStoreResult: erase finished sub-task")
                               << cache->printCurrentState();
                if (m_capacity >= cache->capacity())
                {
                    m_capacity -= cache->capacity();
                }
                else
                {
                    m_capacity = 0;
                }
                continue;
            }
            if (cache->state() == CacheState::Finalized)
            {
                m_config->threadPool()->enqueue([cache]() {
                    // Note: the tryToIntersectionAndStoreResult should not throw exception
                    cache->tryToIntersectionAndStoreResult();
                });
                pclientCipherCache++;
                continue;
            }
            pclientCipherCache++;
        }
        if (clientCipherCache.empty())
        {
            it = m_taskCache.erase(it);
            continue;
        }
        it++;
    }
}

void EcdhCache::eraseTaskCache(std::string const& _taskID)
{
    // erase the taskCache
    // Note: here must using writeLock in case of the capacity been updated multiple times
    bcos::WriteGuard l(x_taskCache);
    if (!m_taskCache.count(_taskID))
    {
        return;
    }
    ECDH_LOG(INFO) << LOG_DESC("eraseCache: erase the taskCache") << LOG_KV("taskID", _taskID);
    // update the capacity
    auto const& subTasks = m_taskCache.at(_taskID);
    for (auto const& it : subTasks)
    {
        auto const& cache = it.second;
        if (m_capacity >= cache->capacity())
        {
            m_capacity -= cache->capacity();
        }
        else
        {
            m_capacity = 0;
        }
    }
    // erase the subTasks
    m_taskCache.erase(_taskID);
}

// Note: only when the task-finished can erase the cache
void EcdhCache::eraseCache(std::string const& _taskID)
{
    auto startT = bcos::utcSteadyTime();
    // Note: since the every DataCache references m_serverDataStore
    // the m_serverDataStore only can be released after all the DataCache have been released
    eraseTaskCache(_taskID);
    // erase the server cipher data cache
    bcos::UpgradableGuard l(x_serverDataStore);
    if (!m_serverDataStore.count(_taskID))
    {
        return;
    }
    ECDH_LOG(INFO) << LOG_DESC("eraseCache: erase the serverDataStore")
                   << LOG_KV("taskID", _taskID);
    bcos::UpgradeGuard ul(l);
    m_serverDataStore.erase(_taskID);
    MallocExtension::instance()->ReleaseFreeMemory();
    ECDH_LOG(INFO) << LOG_DESC("eraseCache") << LOG_KV("taskID", _taskID)
                   << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
}
