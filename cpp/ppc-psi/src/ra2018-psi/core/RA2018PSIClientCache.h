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
 * @file RA2018PSIClientCache.h
 * @author: yujiechen
 * @date 2022-11-8
 */

#pragma once
#include "../../psi-framework/TaskState.h"
#include "../Common.h"
#include "../RA2018PSIConfig.h"
#include "CuckooFilterState.h"
#include "ppc-framework/io/DataBatch.h"
#include "ppc-framework/protocol/DataResource.h"
#include <bcos-utilities/Common.h>
#include <memory>
#include <sstream>

namespace ppc::psi
{
class RA2018PSIClientCache : public std::enable_shared_from_this<RA2018PSIClientCache>
{
public:
    using Ptr = std::shared_ptr<RA2018PSIClientCache>;
    RA2018PSIClientCache(RA2018PSIConfig::Ptr const& _config,
        CuckooFilterState::Ptr _cuckooFilterState, std::string const& _taskID,
        TaskState::Ptr const& _taskState,
        ppc::protocol::DataResource::ConstPtr const& _dataResource, uint32_t _seq,
        ppc::io::DataBatch::Ptr&& _plainData)
      : m_config(_config),
        m_taskID(_taskID),
        m_taskState(_taskState),
        m_cuckooFilterState(std::move(_cuckooFilterState)),
        m_dataResource(_dataResource),
        m_seq(_seq),
        m_plainData(std::move(_plainData))
    {
        m_privateKey = m_config->oprf()->generatePrivateKey();
        m_invPrivateKey = m_config->oprf()->inv(m_privateKey);
    }
    virtual ~RA2018PSIClientCache() = default;

    // the client blind the input data
    virtual std::vector<bcos::bytes> blind();

    // the client finalize the evaluated-data
    virtual void finalize(std::vector<bcos::bytes> const& _evaluatedData);
    // compute the intersection when receive both the finalized data and cuckoo-filter
    virtual void computeIntersection();
    // the data has been intersectioned or not
    virtual CacheState state() const { return m_state; }
    virtual void setState(CacheState _state) { m_state = _state; }

    // store the psi-result after intersectioned
    virtual void storePSIResult();
    // the task-seq
    uint32_t seq() const { return m_seq; }
    // the taskID
    std::string const& taskID() const { return m_taskID; }
    ppc::io::DataBatch::Ptr const& plainData() const { return m_plainData; }
    TaskState::Ptr taskState() const { return m_taskState; }

private:
    inline std::string printCurrentState()
    {
        std::ostringstream stringstream;
        stringstream << LOG_KV("task", m_taskID) << LOG_KV("seq", m_seq)
                     << LOG_KV("plainDataSize", m_plainData->size())
                     << LOG_KV("finalizedData", m_finalizedData.size())
                     << LOG_KV("intersectionData", m_intersectionData.size())
                     << LOG_KV("state", m_state);
        return stringstream.str();
    }

    void syncPSIResult();

private:
    RA2018PSIConfig::Ptr m_config;
    std::string m_taskID;
    TaskState::Ptr m_taskState;
    // Note the data-resource may have multiple cuckoo-filter
    CuckooFilterState::Ptr m_cuckooFilterState;

    ppc::protocol::DataResource::ConstPtr m_dataResource;
    uint32_t m_seq;
    // the plainData batch for given seq
    ppc::io::DataBatch::Ptr m_plainData;

    // the finalized-data
    std::vector<bcos::bytes> m_finalizedData;
    // the intersection-data
    std::vector<bcos::bytes> m_intersectionData;
    CacheState m_state = CacheState::Evaluating;

    // the privateKey
    bcos::bytes m_privateKey;
    bcos::bytes m_invPrivateKey;

    mutable bcos::RecursiveMutex m_mutex;
};
}  // namespace ppc::psi