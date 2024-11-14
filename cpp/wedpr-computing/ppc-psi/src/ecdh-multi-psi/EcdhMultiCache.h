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
 * @file EcdhCache.h
 * @author: zachma
 * @date 2023-5-28
 */
#pragma once
#include "Common.h"
#include "EcdhMultiPSIConfig.h"
#include "ppc-psi/src/Common.h"
#include "ppc-psi/src/psi-framework/TaskState.h"
#include <gperftools/malloc_extension.h>
#include <memory>
#include <sstream>

namespace ppc::psi
{
struct MasterCipherRef
{
    std::set<std::string> refInfo;
    int32_t dataIndex = -1;

    void updateDataIndex(int32_t index)
    {
        if (index == -1)
        {
            return;
        }
        dataIndex = index;
    }
};
/// the master data-cache
class MasterCache
{
public:
    using Ptr = std::shared_ptr<MasterCache>;
    MasterCache(TaskState::Ptr const& taskState, EcdhMultiPSIConfig::Ptr const& config)
      : m_taskState(taskState),
        m_config(config),
        m_peerCount(m_taskState->task()->getAllPeerParties().size())
    {}
    virtual ~MasterCache()
    {
        releaseItersection();
        releaseItersection();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the master cipher datacache destroyed ")
                             << LOG_KV("taskID", m_taskState->task()->id());
    }

    void addCalculatorCipher(std::string _peerId, std::map<uint32_t, bcos::bytes>&& _cipherData,
        uint32_t seq, uint32_t dataBatchCount);

    void addPartnerCipher(std::string _peerId, std::vector<bcos::bytes>&& _cipherData, uint32_t seq,
        uint32_t parternerDataCount);

    bool tryToIntersection();

    std::string printCacheState()
    {
        std::ostringstream stringstream;
        stringstream << LOG_KV("taskID", m_taskState->task()->id())
                     << LOG_KV("CacheState", m_cacheState);
        return stringstream.str();
    }

    std::vector<std::pair<uint64_t, bcos::bytes>> encryptIntersection(bcos::bytes const& randomKey);

private:
    bool shouldIntersection()
    {
        // only evaluating state should intersection
        if (m_cacheState != CacheState::Evaluating)
        {
            return false;
        }
        auto allPeerParties = m_taskState->task()->getAllPeerParties();
        if (allPeerParties.size() == m_finishedPartners.size())
        {
            for (auto const& it : allPeerParties)
            {
                if (!m_finishedPartners.contains(it.first))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    void releaseItersection()
    {
        m_intersecCipher.clear();
        m_intersecCipherIndex.clear();

        // release the intersection information
        std::vector<bcos::bytes>().swap(m_intersecCipher);
        std::vector<uint32_t>().swap(m_intersecCipherIndex);

        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("releaseItersection")
                             << LOG_KV("taskID", m_taskState->task()->id());
    }

    void releaseCache()
    {
        m_masterDataRef.clear();

        // release the parterner cipher
        std::map<bcos::bytes, MasterCipherRef>().swap(m_masterDataRef);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("releaseCache")
                             << LOG_KV("taskID", m_taskState->task()->id());
    }

    void mergeMasterCipher(std::string const& peer);
    void updateMasterDataRef(std::string const& _peerId, bcos::bytes&& data, int32_t dataIndex);

private:
    TaskState::Ptr m_taskState;
    EcdhMultiPSIConfig::Ptr m_config;
    unsigned short m_peerCount;
    CacheState m_cacheState = CacheState::Evaluating;

    // the intersection cipher data of the master
    std::vector<bcos::bytes> m_intersecCipher;
    std::vector<uint32_t> m_intersecCipherIndex;

    std::set<uint32_t> m_calculatorCipherSeqs;
    uint32_t m_calculatorDataBatchCount = 0;

    //  data => refered peers
    std::map<bcos::bytes, MasterCipherRef> m_masterDataRef;

    // partnerId=>received partner seqs
    std::map<std::string, std::set<uint32_t>> m_partnerCipherSeqs;
    // peerId==>dataCount
    std::map<std::string, uint32_t> m_parternerDataCount;
    std::set<std::string> m_finishedPartners;

    bool m_peerMerged = false;

    bcos::Mutex m_mutex;
};

// the cipher ref count
// data ==> {ref count, plainData}
struct CipherRefDetail
{
    unsigned short refCount = 0;
    int32_t plainDataIndex = -1;

    void updatePlainIndex(int32_t index)
    {
        if (index == -1)
        {
            return;
        }
        plainDataIndex = index;
    }
};

class CalculatorCache
{
public:
    using Ptr = std::shared_ptr<CalculatorCache>;
    CalculatorCache(
        TaskState::Ptr const& taskState, bool syncResult, EcdhMultiPSIConfig::Ptr const& config)
      : m_taskState(taskState), m_syncResult(syncResult), m_config(config)
    {}
    virtual ~CalculatorCache()
    {
        releaseDataAfterFinalize();

        m_intersectionResult.clear();
        std::vector<bcos::bytes>().swap(m_intersectionResult);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the calculator cipher datacache destroyed")
                             << LOG_KV("taskID", m_taskState->task()->id());
    }

    bool tryToFinalize();

    bool appendMasterCipher(
        std::vector<bcos::bytes>&& _cipherData, uint32_t seq, uint32_t dataBatchSize);

    void setIntersectionCipher(std::map<uint32_t, bcos::bytes>&& _cipherData);

    void appendPlainData(ppc::io::DataBatch::Ptr const& data)
    {
        bcos::WriteGuard l(x_plainData);
        m_plainData.emplace_back(data);
    }

    std::string printCacheState()
    {
        std::ostringstream stringstream;
        stringstream << LOG_KV("taskID", m_taskState->task()->id())
                     << LOG_KV("CacheState", m_cacheState)
                     << LOG_KV("intersectionSize", m_intersectionResult.size());
        return stringstream.str();
    }

private:
    bcos::bytes getPlainDataByIndex(uint64_t index);
    bool shouldFinalize()
    {
        // only can finalize in Evaluating state
        if (m_cacheState != CacheState::Evaluating)
        {
            return false;
        }
        if (!m_receiveIntersection)
        {
            return false;
        }
        if (m_receivedMasterCipher.size() == 0)
        {
            return false;
        }
        return m_receivedMasterCipher.size() == m_masterDataBatchSize;
    }

    void syncIntersections();

    void releaseDataAfterFinalize()
    {
        for (auto const& it : m_plainData)
        {
            it->release();
        }
        m_cipherRef.clear();
        std::map<bcos::bytes, CipherRefDetail>().swap(m_cipherRef);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("releaseDataAfterFinalize")
                             << LOG_KV("taskID", m_taskState->task()->id());
    }

    void updateCipherRef(bcos::bytes&& data, int32_t index);

private:
    TaskState::Ptr m_taskState;
    bool m_syncResult;
    EcdhMultiPSIConfig::Ptr m_config;
    CacheState m_cacheState = CacheState::Evaluating;

    std::vector<ppc::io::DataBatch::Ptr> m_plainData;
    bcos::SharedMutex x_plainData;

    std::map<bcos::bytes, CipherRefDetail> m_cipherRef;

    // the seqs of the data received from master
    std::set<uint32_t> m_receivedMasterCipher;
    uint32_t m_masterDataBatchSize = 0;
    bool m_receiveAllMasterCipher = false;
    mutable bcos::Mutex m_mutex;

    // the intersection cipher received from master
    bool m_receiveIntersection = false;

    // the final result
    std::vector<bcos::bytes> m_intersectionResult;
};
}  // namespace ppc::psi
