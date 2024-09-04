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
 * @file RA2018PSIClient.h
 * @author: yujiechen
 * @date 2022-11-8
 */

#pragma once
#include "../RA2018PSIConfig.h"
#include "CuckooFilterState.h"
#include "RA2018PSIClientCache.h"
#include <bcos-utilities/CallbackCollectionHandler.h>
#include <bcos-utilities/ThreadPool.h>
#include <unordered_map>
namespace ppc::psi
{
class RA2018PSIClient : public std::enable_shared_from_this<RA2018PSIClient>
{
public:
    using Ptr = std::shared_ptr<RA2018PSIClient>;
    RA2018PSIClient(RA2018PSIConfig::Ptr const& _config, uint64_t maxCacheSize = 1024 * 1024 * 1024)
      : m_config(_config),
        m_commitWorker(std::make_shared<bcos::ThreadPool>("committer", 1)),
        m_maxCacheSize(maxCacheSize)
    {}
    virtual ~RA2018PSIClient() = default;

    // insert new RA2018PSIClientCache for given seq
    std::pair<bool, RA2018PSIClientCache::Ptr> insertCache(
        std::string const& _cuckooFilterResourceID, std::string const& _taskID,
        TaskState::Ptr const& _taskState,
        ppc::protocol::DataResource::ConstPtr const& _dataResource, uint32_t _seq,
        ppc::io::DataBatch::Ptr&& _plainData);

    // erase the cache
    void eraseTaskCache(std::string const& _taskID)
    {
        bcos::WriteGuard l(x_caches);
        m_caches.erase(_taskID);
    }
    // obtain the cache according to the taskID and seq
    RA2018PSIClientCache::Ptr cache(std::string const& _taskID, uint32_t _seq) const;

    // check the task has completed or not, and store the intersection-result
    virtual void checkAndStoreInterSectionResult();
    // compute-intersection
    virtual void computeIntersection();

    virtual CuckooFilterState::Ptr loadCuckooFilterState(
        std::string const& _resourceID, bool _create);

    virtual bool isFull() const { return (m_capacity >= m_maxCacheSize); }

    template <class T>
    bcos::Handler<> onReady(T const& _t)
    {
        return m_onReady.add(_t);
    }

    uint64_t capacity() const { return m_capacity.load(); }

private:
    void commitPSIResult(RA2018PSIClientCache::Ptr _cache);

private:
    RA2018PSIConfig::Ptr m_config;

    // taskID->seq->RA2018PSIClientCache
    using RA2018PSICacheType =
        std::unordered_map<std::string, std::unordered_map<uint32_t, RA2018PSIClientCache::Ptr>>;
    RA2018PSICacheType m_caches;
    mutable bcos::SharedMutex x_caches;

    bcos::ThreadPool::Ptr m_commitWorker;
    uint64_t m_maxCacheSize;

    std::atomic<uint64_t> m_capacity = {0};

    // record the cuckoo-filter-state for given resource
    // TODO: clear the expired resourceCuckooFilterState
    std::map<std::string, CuckooFilterState::Ptr> m_resourceCuckooFilterState;
    mutable bcos::SharedMutex x_resourceCuckooFilterState;

    // onReady
    bcos::CallbackCollectionHandler<> m_onReady;
};
}  // namespace ppc::psi