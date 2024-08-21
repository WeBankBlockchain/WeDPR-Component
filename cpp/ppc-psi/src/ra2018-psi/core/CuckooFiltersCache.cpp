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
 * @file CuckooFiltersCache.cpp
 * @author: yujiechen
 * @date 2022-11-10
 */
#include "CuckooFiltersCache.h"

using namespace ppc::psi;
using namespace bcos;

bool CuckooFiltersCache::insert(
    std::string const& _resourceID, CuckooFilterInfo::Ptr const& _filter)
{
    insertWithoutEvict(_resourceID, std::move(_filter));
    return evict();
}

CuckooFilterInfo::Ptr CuckooFiltersCache::query(std::string const& _resourceID, int32_t _filterID)
{
    auto filterInfo = queryWithoutEvict(_resourceID, _filterID);
    evict();
    return filterInfo;
}

// insert a cuckoo-filter into the cache, flush the last-element from m_resourceQueue if cache-full
void CuckooFiltersCache::insertWithoutEvict(
    std::string const& _resourceID, CuckooFilterInfo::Ptr const& _filter)
{
    UpgradableGuard l(x_mutex);
    auto const& filterID = _filter->filterID();
    auto it = m_resourceToCuckooFilters.find(_resourceID);
    if (it != m_resourceToCuckooFilters.end())
    {
        auto pCuckooFilter = it->second.find(filterID);
        if (pCuckooFilter != it->second.end())
        {
            auto currentFilter = pCuckooFilter->second;
            if (_filter->hash() == currentFilter->hash())
            {
                RA2018_LOG(INFO)
                    << LOG_DESC("CuckooFiltersCache: insert return directly for hit the cache")
                    << LOG_KV("resource", _resourceID) << LOG_KV("filter", filterID)
                    << LOG_KV("capacity", m_capacity);
                return;
            }
            // update the currentFilter if its-hash changed
            RA2018_LOG(INFO) << LOG_DESC("CuckooFiltersCache: update the cuckoo-filter")
                             << LOG_KV("resource", _resourceID) << LOG_KV("filter", filterID)
                             << LOG_KV("capacity", m_capacity);
            _filter->setDirty(true);
            // update the capacity
            m_capacity -= currentFilter->cuckooFilter()->capacity();
            m_capacity += _filter->cuckooFilter()->capacity();
            UpgradeGuard ul(l);
            pCuckooFilter->second = _filter;
            flush(_resourceID, filterID, _filter);
            return;
        }
    }
    UpgradeGuard ul(l);
    // insert new cuckoo-filter
    m_resourceToCuckooFilters[_resourceID][filterID] = _filter;
    m_resourceQueue.push(std::make_pair(_resourceID, filterID));
    _filter->setDirty(true);
    flush(_resourceID, filterID, _filter);
    m_capacity += _filter->cuckooFilter()->capacity();
    RA2018_LOG(INFO) << LOG_DESC("CuckooFiltersCache: insert new cuckoo-filter")
                     << LOG_KV("resource", _resourceID) << LOG_KV("capacity", m_capacity);
}

// query cuckoo-filter specified by hash, access the storage and update the
// m_resourceToCuckooFilters when miss
CuckooFilterInfo::Ptr CuckooFiltersCache::queryWithoutEvict(
    std::string const& _resourceID, int32_t _filterID)
{
    {
        ReadGuard l(x_mutex);
        auto it = m_resourceToCuckooFilters.find(_resourceID);
        if (it != m_resourceToCuckooFilters.end())
        {
            auto pCuckooFilter = it->second.find(_filterID);
            // hit the cache
            if (pCuckooFilter != it->second.end())
            {
                RA2018_LOG(INFO) << LOG_DESC("CuckooFiltersCache: query hit the cache")
                                 << LOG_KV("resource", _resourceID) << LOG_KV("filter", _filterID)
                                 << LOG_KV("capacity", m_capacity);
                return pCuckooFilter->second;
            }
        }
    }
    return loadCuckooFilterFromStorage(_resourceID, _filterID);
}

CuckooFilterInfo::Ptr CuckooFiltersCache::loadCuckooFilterFromStorage(
    std::string const& _resourceID, int32_t _filterID)
{
    try
    {
        RA2018_LOG(INFO) << LOG_DESC("CuckooFiltersCache: loadCuckooFilterFromStorage")
                         << LOG_KV("resource", _resourceID) << LOG_KV("filter", _filterID)
                         << LOG_KV("capacity", m_capacity);
        // miss the cache load the cuckoo-filter from the backend
        // Note: since the load operation may occupy more time, we remove the lock here
        auto filterInfo = m_storage->loadCuckooFilterDataInfo(_resourceID, _filterID, true);
        // the filter non-exist
        if (!filterInfo)
        {
            return nullptr;
        }
        UpgradableGuard l(x_mutex);
        // check the existance again, in case of the filter been inserted
        if (!m_resourceToCuckooFilters.count(_resourceID) ||
            !m_resourceToCuckooFilters.at(_resourceID).count(_filterID))
        {
            UpgradeGuard ul(l);
            filterInfo->setFilterID(_filterID);
            m_resourceToCuckooFilters[_resourceID][_filterID] = filterInfo;
            m_resourceQueue.push(std::make_pair(_resourceID, _filterID));
            m_capacity += filterInfo->cuckooFilter()->capacity();
            return filterInfo;
        }
        return m_resourceToCuckooFilters.at(_resourceID).at(_filterID);
    }
    catch (std::exception const& e)
    {
        // load cuckoo-filter failed
        RA2018_LOG(WARNING) << LOG_DESC("loadCuckooFilterFromStorage exception")
                            << LOG_KV("resource", _resourceID) << LOG_KV("filter", _filterID)
                            << LOG_KV("error", boost::diagnostic_information(e));
        return nullptr;
    }
}

bool CuckooFiltersCache::evict()
{
    // the cache is not full
    if (m_capacity <= m_maxCapacity)
    {
        return false;
    }
    // the cache is full
    bcos::WriteGuard l(x_mutex);
    while (m_capacity > m_maxCapacity)
    {
        // find out the element to evict
        auto element = m_resourceQueue.back();
        m_resourceQueue.pop();
        auto it = m_resourceToCuckooFilters.find(element.first);
        if (it == m_resourceToCuckooFilters.end())
        {
            continue;
        }
        auto pFilter = it->second.find(element.second);
        if (pFilter == it->second.end())
        {
            continue;
        }
        auto evictedCuckooFilter = pFilter->second;
        it->second.erase(pFilter);
        m_capacity -= evictedCuckooFilter->cuckooFilter()->capacity();
        RA2018_LOG(INFO) << LOG_DESC("CuckooFiltersCache evict")
                         << LOG_KV("resource", element.first) << LOG_KV("filter", element.second)
                         << LOG_KV("evictedSize", evictedCuckooFilter->cuckooFilter()->capacity())
                         << LOG_KV("capacity", m_capacity);
        flush(element.first, element.second, evictedCuckooFilter);
    }
    return true;
}

void CuckooFiltersCache::flushAll()
{
    ReadGuard l(x_mutex);
    for (auto const& it : m_resourceToCuckooFilters)
    {
        auto const& resourceID = it.first;
        for (auto const& cuckooItem : it.second)
        {
            flush(resourceID, cuckooItem.first, cuckooItem.second);
        }
    }
}

void CuckooFiltersCache::flush(
    std::string const& _resourceID, int32_t _filterID, CuckooFilterInfo::Ptr _filter)
{
    if (!_filter->dirty())
    {
        return;
    }
    // insert
    RA2018_LOG(INFO) << LOG_DESC("CuckooFiltersCache flush: insert new cuckoo-filter")
                     << LOG_KV("resource", _resourceID) << _filter->desc();
    // Note: since every cuckoo-filter-fetch-process will check the cuckoo-filter between
    // server and client, storage failure has no effect
    m_storage->asyncInsertCuckooFilter(
        _resourceID, _filter->cuckooFilter(),
        [_resourceID](Error::Ptr&& _error) {
            if (!_error)
            {
                return;
            }
            RA2018_LOG(WARNING)
                << LOG_DESC("CuckooFiltersCache evict: insert cuckoo-filter to storage failed")
                << LOG_KV("resource", _resourceID) << LOG_KV("code", _error->errorCode())
                << LOG_KV("msg", _error->errorMessage());
        },
        _filterID);
}
