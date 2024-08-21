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
 * @file CuckoofilterAllocator.cpp
 * @author: yujiechen
 * @date 2022-11-7
 */
#include "CuckoofilterAllocator.h"

using namespace ppc::psi;

// Note: all allocate call will trigger allocate the used-up cuckoo-filter
DefaultCukooFilterPtr CuckoofilterAllocator::allocate()
{
    // with cuckoo-filter and the cuckoo-filter underload
    if (m_currentCuckooFilter)
    {
        if (!m_currentCuckooFilter->cuckooFilter()->full())
        {
            return m_currentCuckooFilter->cuckooFilter();
        }
        else
        {
            // if the cuckoo-filter overload, try to flush to backend
            saveOverLoadedCuckooFilter(false);
        }
    }
    // load the reusable-cuckoo-filter from the backend
    while (!m_currentCuckooFilter && !m_reusableFilters.empty())
    {
        auto const& filterID = m_reusableFilters.back()->filterID();
        try
        {
            // Note: the cuckoo-filter files maybe deleted which will cause exception
            m_currentCuckooFilter =
                m_storage->loadCuckooFilterDataInfo(m_resourceID, filterID, true);
        }
        catch (std::exception const& e)
        {
            RA2018_LOG(WARNING)
                << LOG_DESC("CuckoofilterAllocator: loadCuckooFilterInfo error when allocate")
                << LOG_KV("error", boost::diagnostic_information(e))
                << LOG_KV("resource", m_resourceID) << LOG_KV("filter", filterID);
        }
        m_reusableFilters.pop_back();
    }

    // allocate a new cuckoo-filter
    if (m_reusableFilters.empty() && !m_currentCuckooFilter)
    {
        auto allocatedCuckooFilter =
            std::make_shared<DefaultCukooFilter>(m_config->cuckooFilterOption());
        // the id of the new cuckoo-filter is -1
        m_currentCuckooFilter = std::make_shared<CuckooFilterInfo>(-1);
        m_currentCuckooFilter->setCuckooFilter(std::move(allocatedCuckooFilter));
        RA2018_LOG(INFO) << LOG_DESC("CuckoofilterAllocator: allocate new cuckoofilter");
    }
    else
    {
        RA2018_LOG(INFO) << LOG_DESC("CuckoofilterAllocator: reuse old cuckoofilter: ")
                         << m_currentCuckooFilter->filterID();
    }
    return m_currentCuckooFilter->cuckooFilter();
}

void CuckoofilterAllocator::flush()
{
    RA2018_LOG(INFO) << LOG_DESC("CuckoofilterAllocator flush") << LOG_KV("resource", m_resourceID);
    // Note: the last cuckoo-filter must store to the backend
    saveOverLoadedCuckooFilter(true);
}

void CuckoofilterAllocator::saveOverLoadedCuckooFilter(bool _flush)
{
    // without cuckoo-filter
    if (!m_currentCuckooFilter)
    {
        if (m_flush)
        {
            m_callback(nullptr);
        }
        return;
    }
    m_total++;
    // Note: must set m_total to the real-store-count before trigger the last
    // cuckoo-filter-store-operation
    if (_flush)
    {
        m_flush = true;
    }
    // TODO: check will this cause memory leak here?
    auto allocator = shared_from_this();
    auto callback = [allocator](bcos::Error::Ptr _error) {
        allocator->m_total--;
        if (_error)
        {
            allocator->m_failed++;
        }
        RA2018_LOG(INFO) << LOG_DESC("saveOverLoadedCuckooFilter")
                         << LOG_KV("code", _error ? _error->errorCode() : 0)
                         << LOG_KV("msg", _error ? _error->errorMessage() : "success")
                         << LOG_KV("resource", allocator->m_resourceID);
        // not all data has been stored into backend
        if (allocator->m_total > 0)
        {
            return;
        }
        // not calls flush yet
        if (!allocator->m_flush)
        {
            return;
        }
        // with error
        if (allocator->m_failed)
        {
            allocator->m_callback(std::make_shared<bcos::Error>(-1, "store cuckoo-filter error!"));
            return;
        }
        // success
        allocator->m_callback(nullptr);
    };
    // insert a new-overloaded-cuckoo-filter
    // Note: the m_currentCuckooFilter will be re-allocated later
    auto cuckooFilter = std::move(m_currentCuckooFilter);
    if (-1 == cuckooFilter->filterID())
    {
        RA2018_LOG(INFO) << LOG_DESC(
                                "CuckoofilterAllocator: saveOverLoadedCuckooFilter with insert")
                         << LOG_KV("resourceID", m_resourceID);
        m_storage->asyncInsertCuckooFilter(m_resourceID, cuckooFilter->cuckooFilter(), callback);
        return;
    }
    RA2018_LOG(INFO) << LOG_DESC("CuckoofilterAllocator: saveOverLoadedCuckooFilter with update")
                     << LOG_KV("resourceID", m_resourceID) << cuckooFilter->desc()
                     << LOG_KV("total", m_total);
    // update the old-overloaded-cuckoo-filter
    m_storage->asyncUpdateCuckooFilter(m_resourceID, cuckooFilter, callback);
}