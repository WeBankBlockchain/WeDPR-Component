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
 * @file RA2018PSIClientCache.cpp
 * @author: yujiechen
 * @date 2022-11-8
 */
#include "RA2018PSIClientCache.h"

using namespace ppc::psi;
using namespace bcos;

// the client blind the input data
std::vector<bcos::bytes> RA2018PSIClientCache::blind()
{
    auto startT = utcSteadyTime();
    bcos::RecursiveGuard l(m_mutex);
    std::vector<bcos::bytes> result;
    m_config->oprf()->blind(m_plainData, m_privateKey, result);
    RA2018_LOG(INFO) << LOG_DESC("blind") << LOG_KV("timecost", (utcSteadyTime() - startT))
                     << printCurrentState();
    return result;
}

// the client finalize the evaluated-data
void RA2018PSIClientCache::finalize(std::vector<bcos::bytes> const& _evaluatedData)
{
    bcos::RecursiveGuard l(m_mutex);
    // only can finalize in the evaluating-state
    if (m_state != CacheState::Evaluating)
    {
        return;
    }
    if (_evaluatedData.size() != m_plainData->size())
    {
        RA2018_LOG(WARNING) << LOG_DESC("Invalid evaluated data")
                            << LOG_KV("evaluatedDataSize", _evaluatedData.size())
                            << printCurrentState();
        return;
    }
    m_state = CacheState::Finalizing;

    auto startT = utcSteadyTime();
    std::vector<bcos::bytes> result;
    m_config->oprf()->finalize(_evaluatedData, m_invPrivateKey, m_finalizedData);
    m_state = CacheState::Finalized;
    RA2018_LOG(INFO) << LOG_DESC("finalize successs")
                     << LOG_KV("timecost", (utcSteadyTime() - startT))
                     << LOG_KV("size", m_finalizedData.size()) << printCurrentState();
    return;
}

void RA2018PSIClientCache::computeIntersection()
{
    bcos::RecursiveGuard l(m_mutex);
    // not finalized
    // already been intersectioned or intersection-in-progress or storeing
    if (m_state != CacheState::Finalized)
    {
        RA2018_LOG(DEBUG) << LOG_DESC(
                                 "computeIntersection return for not in-finalized-state(only can "
                                 "intersection in finalized state)")
                          << printCurrentState();
        return;
    }
    // the cuckoo-filters not setted
    if (!m_cuckooFilterState->fetchFinish())
    {
        RA2018_LOG(DEBUG) << LOG_DESC(
                                 "computeIntersection return for the cuckoo-filter not fetched")
                          << printCurrentState();
        return;
    }
    auto startT = utcSteadyTime();
    m_state = CacheState::IntersectionProgressing;
    // trigger intersection
    auto cuckooFilterInfos = m_cuckooFilterState->cuckooFilterInfos();
    for (uint64_t i = 0; i < m_finalizedData.size(); i++)
    {
        auto const& key = m_finalizedData.at(i);
        // foreach the cuckooFilters to find the intersection
        for (auto const& it : cuckooFilterInfos)
        {
            if (it.second->cuckooFilter()->contains(key))
            {
                m_intersectionData.emplace_back(m_plainData->getBytes(i));
                break;
            }
        }
    }
    m_state = CacheState::Intersectioned;
    RA2018_LOG(INFO) << LOG_DESC("computeIntersection success")
                     << LOG_KV("timecost", (utcSteadyTime() - startT)) << printCurrentState()
                     << LOG_KV("intersectionSize", m_intersectionData.size())
                     << LOG_KV("cuckooFilterSize", cuckooFilterInfos.size());
    return;
}

void RA2018PSIClientCache::storePSIResult()
{
    // only can store the intersectioned data
    bcos::RecursiveGuard l(m_mutex);
    if (m_state != CacheState::Intersectioned)
    {
        return;
    }
    m_state = CacheState::StoreProgressing;
    RA2018_LOG(INFO) << LOG_DESC("storePSIResult") << printCurrentState()
                     << printDataResourceInfo(m_dataResource);
    // store the psi result
    m_taskState->storePSIResult(m_config->dataResourceLoader(), m_intersectionData);
    m_state = CacheState::Stored;
    // commit success
    if (!m_taskState->task()->syncResultToPeer())
    {
        m_state = CacheState::Synced;
        // No need to sync task to peers, erase the seq from the task-state directly
        m_taskState->eraseFinishedTaskSeq(m_seq, true);
        return;
    }
    // sync psi result to server
    syncPSIResult();

    // release the allocated memory
    std::vector<bcos::bytes>().swap(m_finalizedData);
    std::vector<bcos::bytes>().swap(m_intersectionData);
}

// sync psi result to server
void RA2018PSIClientCache::syncPSIResult()
{
    if (m_state != CacheState::Stored)
    {
        return;
    }
    m_state = CacheState::Syncing;
    RA2018_LOG(INFO) << LOG_DESC("syncResultToPeer") << printCurrentState()
                     << LOG_KV("peer", m_taskState->peerID());
    auto dataSyncMsg =
        m_config->ra2018MsgFactory()->createPSIMessage((uint32_t)PSIPacketType::PSIResultSyncMsg);
    dataSyncMsg->setData(m_intersectionData);
    auto self = weak_from_this();
    m_config->generateAndSendPPCMessage(
        m_taskState->peerID(), m_taskState->task()->id(), dataSyncMsg,
        [self](Error::Ptr&& _error) {
            auto cache = self.lock();
            if (!cache)
            {
                return;
            }
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            RA2018_LOG(WARNING) << LOG_DESC("Sync PSI result error") << LOG_KV("seq", cache->m_seq)
                                << printTaskInfo(cache->m_taskState->task());
        },
        m_seq,
        [self](bcos::Error::Ptr _error, std::string const& _agencyID,
            ppc::front::PPCMessageFace::Ptr _msg, ppc::front::ResponseFunc) {
            auto cache = self.lock();
            if (!cache)
            {
                return;
            }
            if (_error)
            {
                RA2018_LOG(WARNING)
                    << LOG_DESC("Sync PSI result error") << LOG_KV("seq", cache->m_seq)
                    << printTaskInfo(cache->m_taskState->task());
            }
            else
            {
                RA2018_LOG(INFO) << LOG_DESC("Sync PSI result success")
                                 << LOG_KV("seq", cache->m_seq)
                                 << printTaskInfo(cache->m_taskState->task());
            }
            cache->m_state = CacheState::Synced;
            // erase the seq from task-state only when receive the response from the peer
            cache->m_taskState->eraseFinishedTaskSeq(cache->m_seq, true);
        });
}
