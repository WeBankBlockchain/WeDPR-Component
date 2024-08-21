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
 * @file CuckoofilterAllocator.h
 * @author: yujiechen
 * @date 2022-11-7
 */
#pragma once
#include "../Common.h"
#include "../RA2018PSIConfig.h"
#include "../storage/RA2018PSIStorage.h"

namespace ppc::psi
{
class CuckoofilterAllocator : public std::enable_shared_from_this<CuckoofilterAllocator>
{
public:
    // the _reusableFilters record the cuckoo-filter ids that not full
    CuckoofilterAllocator(RA2018PSIConfig::Ptr _config, RA2018PSIStorage::Ptr _storage,
        std::string const& _resourceID, std::function<void(bcos::Error::Ptr&&)> _callback,
        CuckooFilterInfoSet&& _reusableFilters)
      : m_config(std::move(_config)),
        m_storage(std::move(_storage)),
        m_resourceID(_resourceID),
        m_callback(std::move(_callback)),
        m_reusableFilters(_reusableFilters.begin(), _reusableFilters.end())
    {}
    virtual ~CuckoofilterAllocator() = default;

    // allocate the underload cuckoo-filter
    virtual DefaultCukooFilterPtr allocate();
    // flush the updated/new cuckoo-filter into the backend-storage
    // Note: the callback can'be called only after flush
    virtual void flush();

private:
    void saveOverLoadedCuckooFilter(bool _flush);

private:
    RA2018PSIConfig::Ptr m_config;
    RA2018PSIStorage::Ptr m_storage;
    std::string m_resourceID;
    std::function<void(bcos::Error::Ptr&&)> m_callback;

    std::vector<CuckooFilterInfo::Ptr> m_reusableFilters;

    // the current allocated cuckoo-filter
    CuckooFilterInfo::Ptr m_currentCuckooFilter = nullptr;

    // for determine store the cuckoo-filter success or not
    std::atomic<int> m_total = {0};
    std::atomic<int> m_failed = {0};
    std::atomic<bool> m_flush = {false};
};
}  // namespace ppc::psi