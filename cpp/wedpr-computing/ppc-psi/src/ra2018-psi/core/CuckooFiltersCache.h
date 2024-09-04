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
 * @file CuckooFiltersCache.h
 * @author: yujiechen
 * @date 2022-11-10
 */
#pragma once
#include "../storage/RA2018PSIStorage.h"
#include "CuckooFilterInfo.h"
#include <bcos-utilities/Common.h>
#include <queue>
namespace ppc::psi
{
// Note: the cuckoo-filter is useful for the client
//       we access the storage directly when the save insert/update/load cuckoo-filter
class CuckooFiltersCache
{
public:
    using Ptr = std::shared_ptr<CuckooFiltersCache>;
    // default cache size is 255MB
    CuckooFiltersCache(
        RA2018PSIStorage::Ptr const& _storage, uint64_t _maxCapacity = 255 * 1024 * 1024)
      : m_storage(_storage), m_maxCapacity(_maxCapacity)
    {}
    virtual ~CuckooFiltersCache() {}

    // insert a cuckoo-filter into the cache, flush the last-element from m_resourceQueue if
    // cache-full, return the cache is full or not
    virtual bool insert(std::string const& _resourceID, CuckooFilterInfo::Ptr const& _filter);

    // query cuckoo-filter specified by hash, access the storage and update the
    // m_resourceToCuckooFilters when miss
    virtual CuckooFilterInfo::Ptr query(std::string const& _resourceID, int32_t _filterID);

    // flush the updated/dirty cuckoo-filter into the db
    // we can call this periodly
    virtual void flushAll();

    uint64_t capacity() const { return m_capacity; }
    bool full() const { return m_capacity >= m_maxCapacity; }

protected:
    // Note: only flush the dirty-cuckoo-filter to the backend when evict
    virtual bool evict();
    // insert the cuckoo-filter into the db
    virtual void insertWithoutEvict(
        std::string const& _resourceID, CuckooFilterInfo::Ptr const& _filter);

    // query the cuckoo-filter according to the resourceID and filterID
    virtual CuckooFilterInfo::Ptr queryWithoutEvict(
        std::string const& _resourceID, int32_t _filterID);

    // load the cuckoo-filter from the backend-storage
    virtual CuckooFilterInfo::Ptr loadCuckooFilterFromStorage(
        std::string const& _resourceID, int32_t _filterID);

    // flush the dirty/inserted cuckoo-filter to the storage
    virtual void flush(
        std::string const& _resourceID, int32_t _filterID, CuckooFilterInfo::Ptr _filter);

private:
    RA2018PSIStorage::Ptr m_storage;
    uint64_t m_maxCapacity;
    std::atomic<uint64_t> m_capacity = {0};

    // for FIFO cache-policy
    // (resourceID, filter ID)
    std::queue<std::pair<std::string, uint32_t>> m_resourceQueue;
    // the resourceID to cuckoo-filters
    // resourceID => filter id => cuckoo-filter
    std::map<std::string, std::map<uint32_t, CuckooFilterInfo::Ptr>> m_resourceToCuckooFilters;
    mutable bcos::SharedMutex x_mutex;
};
}  // namespace ppc::psi