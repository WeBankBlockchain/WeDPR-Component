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
#include "ppc-psi/src/Common.h"
#include "ppc-psi/src/psi-framework/TaskState.h"
#include <gperftools/malloc_extension.h>
#include <memory>

namespace ppc::psi
{
/// the master data-cache
class MasterCache
{
public:
    using Ptr = std::shared_ptr<MasterCache>;
    MasterCache(TaskState::Ptr const& taskState) : m_taskState(taskState) {}
    virtual ~MasterCache()
    {
        m_intersecCipher.clear();
        m_finishedPartners.clear();
        m_calculatorCipher.clear();
        m_partnerToCipher.clear();
        m_calculatorCipherSeqs.clear();
        m_partnerCipherSeqs.clear();
        std::map<uint32_t, bcos::bytes>().swap(m_intersecCipher);
        std::set<std::string>().swap(m_finishedPartners);
        std::map<uint32_t, bcos::bytes>().swap(m_calculatorCipher);
        std::map<std::string, std::vector<bcos::bytes>>().swap(m_partnerToCipher);
        std::set<uint32_t>().swap(m_calculatorCipherSeqs);
        std::map<std::string, std::set<uint32_t>>().swap(m_partnerCipherSeqs);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the master cipher datacache destroyed ")
                             << LOG_KV("taskID", m_taskID);
    }

    void addCalculatorCipher(std::string _peerId, std::map<uint32_t, bcos::bytes>&& _cipherData,
        uint32_t seq, uint32_t dataBatchCount);

    void addPartnerCipher(std::string _peerId, std::vector<bcos::bytes>&& _cipherData, uint32_t seq,
        uint32_t needSendTimes);

    // Note: the m_intersecCipher will not been changed once generated
    std::map<uint32_t, bcos::bytes> const& intersecCipher() { return m_intersecCipher; }

    bool tryToIntersection();

    std::string printCacheState()
    {
        std::ostringstream stringstream;
        stringstream << LOG_KV("taskID", m_taskState->task()->taskID())
                     << LOG_KV("CacheState", CacheState)
                     << LOG_KV("intersectionSize", m_intersecCipher.size());
        if (_msg->header()->hasOptionalField())
        {
            stringstream << printOptionalField(_msg->header()->optionalField());
        }
        return stringstream.str();
    }

    std::vector<bcos::bytes> encryptIntersection(bcos::bytes const& randomKey);

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

private:
    TaskState::Ptr m_taskState;
    CacheState m_cacheState = CacheState::Evaluating;

    // the intersection cipher data of the master
    // calculator data index ==> cipher
    std::map<uint32_t, bcos::bytes> m_intersecCipher;
    std::set<std::string> m_finishedPartners;
    // the cipher data from calculator to master
    std::map<uint32_t, bcos::bytes> m_calculatorCipher;
    uint32_t m_calculatorDataBatchCount = 0;
    std::set<uint32_t> m_calculatorCipherSeqs;
    bcos::SharedMutex x_calculatorCipher;

    // TODO: replace with unordered_set
    std::map<std::string, std::set<bcos::bytes>> m_partnerToCipher;
    bcos::SharedMutex x_partnerToCipher;
    // partnerId=>received partner seqs
    std::map<std::string, std::set<uint32_t>> m_partnerCipherSeqs;

    bcos::Mutex m_mutex;
};

class CalculatorCache
{
public:
    using Ptr = std::shared_ptr<CalculatorCache>;
    CalculatorCache(TaskState::Ptr const& taskState, bool syncResult)
      : m_taskState(taskState), m_syncResult(syncResult)
    {}
    virtual ~CalculatorCache()
    {
        m_CipherDataFromCalculatorSubSeq.clear();
        m_calculatorIntersectionSubSeq.clear();
        m_receivedMasterCipher.clear();
        m_masterCipher.clear();
        m_intersectionCipher.clear();
        m_calculatorIntersectionCipherDataFinalMap.clear();
        std::set<uint32_t>().swap(m_CipherDataFromCalculatorSubSeq);
        std::set<uint32_t>().swap(m_calculatorIntersectionSubSeq);
        std::set<uint32_t>().swap(m_receivedMasterCipher);
        std::vector<bcos::bytes>().swap(m_masterCipher);
        std::map<uint32_t, bcos::bytes>().swap(m_intersectionCipher);
        std::map<uint32_t, bcos::bytes>().swap(m_calculatorIntersectionCipherDataFinalMap);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the calculator cipher datacache destroyed")
                             << LOG_KV("taskID", m_taskID);
    }

    void tryToFinalize();

    bool appendMasterCipher(
        std::vector<bcos::bytes>&& _cipherData, uint32_t seq, uint32_t dataBatchSize);

    void setIntersectionCipher(std::map<uint32_t, bcos::bytes>&& _cipherData);

    std::map<uint32_t, bcos::bytes> const& calculatorIntersectionCipherDataFinalMap()
    {
        return m_calculatorIntersectionCipherDataFinalMap;
    }


    void appendPlainData(ppc::io::DataBatch::Ptr const& data)
    {
        bcos::WriteGuard l(x_plainData);
        m_plainData->emplace_back(data);
    }

private:
    bool shouldFinalize()
    {
        // only can finalize in Evaluating state
        if (m_state != CacheState::Evaluating)
        {
            return false;
        }
        return m_receivedMasterCipher.size() == m_masterDataBatchSize;
    }

    void syncIntersections();

private:
    TaskState::Ptr m_taskState;
    CacheState m_cacheState = CacheState::Evaluating;
    bool m_syncResult;

    std::vector<ppc::io::DataBatch::Ptr> m_plainData;
    bcos::SharedMutex x_plainData;


    // store the cipher-data of the calculator
    std::set<uint32_t> m_CipherDataFromCalculatorSubSeq;
    std::set<uint32_t> m_calculatorIntersectionSubSeq;


    // the cipher from the master
    std::set<bcos::bytes> m_masterCipher;
    // the seqs of the data received from master
    std::set<uint32_t> m_receivedMasterCipher;
    uint32_t m_masterDataBatchSize = 0;
    mutable bcos::SharedMutex x_masterCipher;

    // the intersection cipher received from master
    std::map<uint32_t, bcos::bytes> m_intersectionCipher;
    mutable bcos::SharedMutex x_intersectionCipher;
    std::vector<bcos::bytes> m_intersectionResult;
};
}  // namespace ppc::psi