/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file FakeRA2018PSIStorage.h
 * @author: yujiechen
 * @date 2022-11-16
 */
#pragma once
#include "ppc-crypto-core/src/hash/MD5Hash.h"
#include "ppc-psi/src/ra2018-psi/storage/RA2018PSIStorage.h"

using namespace ppc::psi;
using namespace ppc::crypto;

namespace ppc::test
{
class FakeRA2018PSIStorage : public RA2018PSIStorage
{
public:
    FakeRA2018PSIStorage(RA2018PSIConfig::Ptr _config)
      : RA2018PSIStorage(_config), m_hashImpl(std::make_shared<MD5Hash>())
    {}

    ~FakeRA2018PSIStorage() override = default;

    void init() override {}
    // get the (cuckoo-filter-id, cuckoo-filter-hash according to the _resourceID and loadFactor
    // TODO: optimize the perf(use async-client)
    CuckooFilterInfoSet getCuckooFilterInfos(
        std::string const& _resourceID, bool _onlyNotFull) override
    {
        CuckooFilterInfoSet result;
        bcos::ReadGuard l(x_cuckooFilterInfo);
        if (!m_cuckooFilterInfo.count(_resourceID))
        {
            return result;
        }
        auto const& filters = m_cuckooFilterInfo.at(_resourceID);
        for (auto const& it : filters)
        {
            auto filter = it.second->cuckooFilter();
            if (_onlyNotFull && !filter->full())
            {
                result.insert(it.second);
                continue;
            }
            result.insert(it.second);
        }
        return result;
    }

    // load the cuckoo-filter from the storage
    CuckooFilterInfo::Ptr loadCuckooFilterDataInfo(
        std::string const& _resourceID, int32_t _filterID, bool _deserialize) override
    {
        bcos::ReadGuard l(x_cuckooFilterInfo);
        if (!m_cuckooFilterInfo.count(_resourceID) ||
            !m_cuckooFilterInfo.at(_resourceID).count(_filterID))
        {
            return nullptr;
        }
        auto filterInfo = m_cuckooFilterInfo[_resourceID][_filterID];
        // with raw-data
        if (!_deserialize)
        {
            auto cuckooFilterInfo =
                std::make_shared<CuckooFilterInfo>(filterInfo->filterID(), filterInfo->hash());
            auto encodedData = filterInfo->cuckooFilter()->serialize();
            bcos::bytes data(encodedData.begin(), encodedData.end());
            // set the cuckoo-filterData
            cuckooFilterInfo->setCuckooFilterData(std::move(data));
            return cuckooFilterInfo;
        }
        // with deserialized cuckoo-filter
        return filterInfo;
    }

    // insert a new cuckoo-filter to the storage
    // update the entry if key-conflict
    void asyncInsertCuckooFilter(std::string const& _resourceID,
        DefaultCukooFilterPtr _cuckooFilter, std::function<void(bcos::Error::Ptr&&)> _callback,
        int32_t _filterID = -1) override
    {
        auto filterID = _filterID;
        bcos::WriteGuard l(x_cuckooFilterInfo);
        if (filterID == -1)
        {
            m_filterID++;
            filterID = m_filterID;
        }
        m_cuckooFilterInfo[_resourceID][filterID] =
            generateCuckooFilterInfo(filterID, _cuckooFilter);
        _callback(nullptr);
    }

    // update the cuckoo-filter
    // Note: update is more efficient than asyncInsertCuckooFilter
    void asyncUpdateCuckooFilter(std::string const& _resourceID,
        CuckooFilterInfo::Ptr const& _filterInfo,
        std::function<void(bcos::Error::Ptr&&)> _callback) override
    {
        bcos::WriteGuard l(x_cuckooFilterInfo);
        if (!m_cuckooFilterInfo.count(_resourceID) ||
            !m_cuckooFilterInfo.at(_resourceID).count(_filterInfo->filterID()))
        {
            _callback(std::make_shared<bcos::Error>(
                -1, "update cuckoo-filter failed for the entry not exist!"));
            return;
        }
        m_cuckooFilterInfo[_resourceID][_filterInfo->filterID()] =
            generateCuckooFilterInfo(_filterInfo->filterID(), _filterInfo->cuckooFilter());
        _callback(nullptr);
    }

private:
    CuckooFilterInfo::Ptr generateCuckooFilterInfo(
        int32_t _filterID, DefaultCukooFilterPtr _cuckooFilter)
    {
        auto encodedData = _cuckooFilter->serialize();
        auto hashResult = m_hashImpl->hash(
            bcos::bytesConstRef((bcos::byte*)encodedData.data(), encodedData.size()));
        auto cuckooFilterInfo = std::make_shared<CuckooFilterInfo>(_filterID, hashResult);
        cuckooFilterInfo->setCuckooFilter(std::move(_cuckooFilter));
        return cuckooFilterInfo;
    }

private:
    Hash::Ptr m_hashImpl;
    // resourceID => filterID => cuckooFilter
    std::map<std::string, std::map<int32_t, CuckooFilterInfo::Ptr>> m_cuckooFilterInfo;
    mutable bcos::SharedMutex x_cuckooFilterInfo;
    int32_t m_filterID = 0;
};
}  // namespace ppc::test