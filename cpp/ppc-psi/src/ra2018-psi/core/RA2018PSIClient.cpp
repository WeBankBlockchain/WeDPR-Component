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
 * @file RA2018PSIClient.cpp
 * @author: yujiechen
 * @date 2022-11-8
 */
#include "RA2018PSIClient.h"
#include <boost/exception/diagnostic_information.hpp>

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace bcos;

// insert new RA2018PSIClientCache for given task and seq
std::pair<bool, RA2018PSIClientCache::Ptr> RA2018PSIClient::insertCache(
    std::string const& _cuckooFilterResourceID, std::string const& _taskID,
    TaskState::Ptr const& _taskState, DataResource::ConstPtr const& _dataResource, uint32_t _seq,
    ppc::io::DataBatch::Ptr&& _plainData)
{
    if (isFull())
    {
        return std::make_pair(false, nullptr);
    }
    bcos::UpgradableGuard l(x_caches);
    auto it = m_caches.find(_taskID);
    if (it != m_caches.end())
    {
        auto pItem = it->second.find(_seq);
        if (pItem != it->second.end())
        {
            RA2018_LOG(WARNING) << LOG_DESC("insertCache return for the cache already exists")
                                << LOG_KV("task", _taskID) << LOG_KV("seq", _seq);
            return std::make_pair(false, pItem->second);
        }
    }
    bcos::UpgradeGuard ul(l);
    // TODO: Be careful of the memory expansion caused by the evaluated-data and intersection-data
    // stored in the RA2018PSIClientCache
    m_capacity += _plainData->capacityBytes();
    auto state = loadCuckooFilterState(_cuckooFilterResourceID, false);
    auto cacheItem = std::make_shared<RA2018PSIClientCache>(
        m_config, state, _taskID, _taskState, _dataResource, _seq, std::move(_plainData));
    m_caches[_taskID][_seq] = cacheItem;
    RA2018_LOG(INFO) << LOG_DESC("insertCache success") << LOG_KV("task", _taskID)
                     << LOG_KV("seq", _seq);
    return std::make_pair(true, cacheItem);
}

// obtain the cache according to the taskID and seq
RA2018PSIClientCache::Ptr RA2018PSIClient::cache(std::string const& _taskID, uint32_t _seq) const
{
    bcos::ReadGuard l(x_caches);
    if (!m_caches.count(_taskID) || !m_caches.at(_taskID).count(_seq))
    {
        return nullptr;
    }
    return m_caches.at(_taskID).at(_seq);
}

// Serially write the results of each (task, seq) to the result file
// erase the finished (task, seq) cache from the m_caches
void RA2018PSIClient::checkAndStoreInterSectionResult()
{
    bcos::WriteGuard l(x_caches);
    for (auto it = m_caches.begin(); it != m_caches.end();)
    {
        auto& taskCache = it->second;
        for (auto ptaskCache = taskCache.begin(); ptaskCache != taskCache.end();)
        {
            // commit the psi-result and erase the intersectioned cache
            auto state = ptaskCache->second->state();
            if (state == CacheState::Intersectioned)
            {
                // Note: commit for the same-task must be seq when the output-data-resource is file
                commitPSIResult(ptaskCache->second);
                auto plainDataSize = ptaskCache->second->plainData()->capacityBytes();
                if (plainDataSize > m_capacity)
                {
                    m_capacity = 0;
                }
                else
                {
                    m_capacity -= plainDataSize;
                }
                // notify to wakeup the worker when the capacity decreased
                m_onReady();
            }
            // Note: only the can erase cache with synced state
            if (state == CacheState::Synced)
            {
                ptaskCache = taskCache.erase(ptaskCache);
                continue;
            }
            ptaskCache++;
        }
        // all seq-data of the task has been handled finished
        if (taskCache.empty())
        {
            it = m_caches.erase(it);
            continue;
        }
        it++;
    }
}

// trigger computeIntersection when receive the cuckoo-filter-response
void RA2018PSIClient::computeIntersection()
{
    auto self = weak_from_this();
    ReadGuard l(x_caches);
    for (auto const& it : m_caches)
    {
        auto clientCaches = it.second;
        for (auto const& item : clientCaches)
        {
            auto clientCache = item.second;
            auto state = clientCache->state();
            // only trigger intersection when finalized
            // trigger psi-store when intersectioned
            if (state != CacheState::Finalized && state != CacheState::Intersectioned)
            {
                continue;
            }
            m_config->threadPool()->enqueue([self, clientCache]() {
                auto client = self.lock();
                if (!client)
                {
                    return;
                }
                try
                {
                    clientCache->computeIntersection();
                    client->checkAndStoreInterSectionResult();
                }
                catch (std::exception const& e)
                {
                    RA2018_LOG(WARNING)
                        << LOG_DESC("computeIntersection exception, cancel the task")
                        << LOG_KV("exception", boost::diagnostic_information(e))
                        << printTaskInfo(clientCache->taskState()->task());
                    clientCache->taskState()->onTaskException(boost::diagnostic_information(e));
                }
            });
        }
    }
}

// TODO: optimize the commit Performance with multiple thread
void RA2018PSIClient::commitPSIResult(RA2018PSIClientCache::Ptr _cache)
{
    // in-case of store the result multiple times
    auto self = weak_from_this();
    m_commitWorker->enqueue([_cache, self]() {
        auto client = self.lock();
        if (!client)
        {
            return;
        }
        try
        {
            auto startT = utcSteadyTime();
            _cache->storePSIResult();
            // notify to check the task-result
            RA2018_LOG(INFO) << LOG_DESC("commitPSIResult success")
                             << LOG_KV("timecost", (utcSteadyTime() - startT))
                             << LOG_KV("task", _cache->taskID()) << LOG_KV("seq", _cache->seq());
        }
        catch (std::exception const& e)
        {
            // commit error
            _cache->taskState()->eraseFinishedTaskSeq(_cache->seq(), false);
            RA2018_LOG(WARNING) << LOG_DESC("commitPSIResult error")
                                << LOG_KV("task", _cache->taskID()) << LOG_KV("seq", _cache->seq())
                                << LOG_KV("msg", boost::diagnostic_information(e));
        }
    });
}

CuckooFilterState::Ptr RA2018PSIClient::loadCuckooFilterState(
    std::string const& _resourceID, bool _create)
{
    bcos::UpgradableGuard l(x_resourceCuckooFilterState);
    auto it = m_resourceCuckooFilterState.find(_resourceID);
    if (it != m_resourceCuckooFilterState.end())
    {
        return it->second;
    }
    if (!_create)
    {
        return nullptr;
    }
    // create and insert a new cuckoo-filter-state
    auto state = std::make_shared<CuckooFilterState>(_resourceID);
    RA2018_LOG(INFO) << LOG_DESC("loadCuckooFilterState: create cuckooFilterState")
                     << LOG_KV("resource", _resourceID);
    bcos::UpgradeGuard ul(l);
    m_resourceCuckooFilterState[_resourceID] = state;
    return state;
}
