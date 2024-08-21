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
 * @file CuckooFilterInfo.h
 * @author: yujiechen
 * @date 2022-11-10
 */
#pragma once
#include "../Common.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <memory>

namespace ppc::psi
{
class CuckooFilterInfo
{
public:
    using Ptr = std::shared_ptr<CuckooFilterInfo>;
    CuckooFilterInfo(int32_t _filterID) : m_filterID(_filterID) {}
    CuckooFilterInfo(int32_t _filterID, bcos::bytes const& _hash)
      : m_filterID(_filterID), m_hash(_hash)
    {}

    CuckooFilterInfo(
        int32_t _filterID, bcos::bytes const& _hash, DefaultCukooFilterPtr const& _cuckooFilter)
      : CuckooFilterInfo(_filterID, _hash)
    {
        m_cuckooFilter = _cuckooFilter;
    }

    int32_t filterID() const { return m_filterID; }
    bcos::bytes const& hash() const { return m_hash; }

    DefaultCukooFilterPtr cuckooFilter() const { return m_cuckooFilter; }

    bcos::bytes const& cuckooFilterData() const { return m_cuckooFilterData; }

    bool dirty() const { return m_dirty; }

    void setDirty(bool _dirty) { m_dirty = _dirty; }
    void setCuckooFilter(DefaultCukooFilterPtr&& _cuckooFilter)
    {
        m_cuckooFilter = std::move(_cuckooFilter);
    }
    void setCuckooFilterData(bcos::bytes&& _data) { m_cuckooFilterData = std::move(_data); }
    void setFilterID(int32_t _filterID) { m_filterID = _filterID; }

    void setHash(bcos::bytes const& _hash) { m_hash = _hash; }
    void setHash(bcos::bytes&& _hash) { m_hash = std::move(_hash); }

    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("id", m_filterID) << LOG_KV("hash", bcos::toHex(m_hash));
        return oss.str();
    }

private:
    int32_t m_filterID;
    bcos::bytes m_hash;
    // Note: in some cases, no need to deserialize the cuckoo-filter
    DefaultCukooFilterPtr m_cuckooFilter;
    bcos::bytes m_cuckooFilterData;

    bool m_dirty = false;

    bcos::bytes m_emptyCuckooFilterData;
};

struct CuckooFilterInfoComparator
{
    bool operator()(const CuckooFilterInfo::Ptr& _left, const CuckooFilterInfo::Ptr& _right) const
    {
        if (_left->filterID() == _right->filterID())
        {
            return _left->hash() < _right->hash();
        }
        return (_left->filterID() < _right->filterID());
    }
};
using CuckooFilterInfoSet = std::set<CuckooFilterInfo::Ptr, CuckooFilterInfoComparator>;
}  // namespace ppc::psi