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
 * @file CuckooFilterState.h
 * @author: yujiechen
 * @date 2022-11-8
 */
#pragma once
#include "../Common.h"
#include <memory>

namespace ppc::psi
{
class CuckooFilterState
{
public:
    using Ptr = std::shared_ptr<CuckooFilterState>;
    CuckooFilterState(std::string const& _resourceID) : m_resourceID(_resourceID) {}
    virtual ~CuckooFilterState() = default;

    std::string const& resourceID() const { return m_resourceID; }
    // Note: the state will only been accessed by one task with _resourceID
    std::map<int32_t, CuckooFilterInfo::Ptr> const& cuckooFilterInfos() const
    {
        bcos::ReadGuard l(x_cuckooFilterInfos);
        return m_cuckooFilterInfos;
    }
    // the resource fetched all cuckoo-filters or not
    bool fetchFinish() const
    {
        if (m_cuckooFilterSize.load() == -1)
        {
            return false;
        }
        bcos::ReadGuard l(x_receivedCuckooFilter);
        if ((int32_t)m_receivedCuckooFilter.size() == m_cuckooFilterSize.load())
        {
            return true;
        }
        return false;
    }

    void decreaseCuckooFilterSize(int32_t _decreasedCuckooFilterSize)
    {
        m_cuckooFilterSize -= _decreasedCuckooFilterSize;
    }

    void init()
    {
        bcos::WriteGuard lock(x_receivedCuckooFilter);
        m_receivedCuckooFilter.clear();
        m_cuckooFilterSize.store(-1);
    }

    void appendCuckooFilterInfo(uint32_t _seq, CuckooFilterInfo::Ptr&& _cuckooFilterInfo)
    {
        {
            bcos::WriteGuard lock(x_receivedCuckooFilter);
            m_receivedCuckooFilter.insert(_seq);
            RA2018_LOG(INFO) << LOG_DESC("appendCuckooFilterInfo") << LOG_KV("seq", _seq)
                             << LOG_KV("receivedSize", m_receivedCuckooFilter.size())
                             << LOG_KV("expectedSize", m_cuckooFilterSize);
        }
        auto const& filterID = _cuckooFilterInfo->filterID();
        // Note: not update the old-cuckoo-filter when hit
        bcos::UpgradableGuard l(x_cuckooFilterInfos);
        if (m_cuckooFilterInfos.count(filterID) &&
            m_cuckooFilterInfos.at(filterID)->hash() == _cuckooFilterInfo->hash())
        {
            return;
        }
        bcos::UpgradeGuard ul(l);
        m_cuckooFilterInfos[filterID] = _cuckooFilterInfo;
    }

    void setCuckooFilterSize(int32_t _cuckooFilterSize)
    {
        m_cuckooFilterSize.store(_cuckooFilterSize);
    }

private:
    std::string m_resourceID;
    // the total-cuckoo-filters of the given resourceID
    std::atomic<int32_t> m_cuckooFilterSize = {0};

    // record the loaded cuckoo-filters, update when the filterID hitted
    std::map<int32_t, CuckooFilterInfo::Ptr> m_cuckooFilterInfos;
    mutable bcos::SharedMutex x_cuckooFilterInfos;

    // record the fetched cuckoo-filter
    std::set<int32_t> m_receivedCuckooFilter;
    mutable bcos::SharedMutex x_receivedCuckooFilter;
};
}  // namespace ppc::psi