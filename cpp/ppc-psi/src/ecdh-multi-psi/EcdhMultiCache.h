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
#include "ppc-psi/src/psi-framework/TaskState.h"
#include <gperftools/malloc_extension.h>
#include <memory>

namespace ppc::psi
{
/// the master data-cache
class MasterCipherDataCache
{
public:
    using Ptr = std::shared_ptr<MasterCipherDataCache>;
    MasterCipherDataCache() {}
    virtual ~MasterCipherDataCache()
    {
        m_masterFinalIntersectionCipherData.clear();
        m_masterTaskPeersFinishedList.clear();
        m_masterCipherDataFromCalculator.clear();
        m_masterCipherDataFromPartner.clear();
        m_CipherDataFromCalculatorSubSeq.clear();
        m_CipherDataFromPartnerSubSeq.clear();
        std::map<uint32_t, bcos::bytes>().swap(m_masterFinalIntersectionCipherData);
        std::set<std::string>().swap(m_masterTaskPeersFinishedList);
        std::map<uint32_t, bcos::bytes>().swap(m_masterCipherDataFromCalculator);
        std::map<std::string, std::vector<bcos::bytes>>().swap(m_masterCipherDataFromPartner);
        std::set<uint32_t>().swap(m_CipherDataFromCalculatorSubSeq);
        std::map<std::string, std::set<uint32_t>>().swap(m_CipherDataFromPartnerSubSeq);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the master cipher datacache destroyed ")
                             << LOG_KV("taskID", m_taskID);
    }

    void appendMasterCipherDataFromCalculator(std::string _peerId,
        std::map<uint32_t, bcos::bytes>&& _cipherData, uint32_t seq, uint32_t needSendTimes)
    {
        try
        {
            bcos::WriteGuard lock(x_peerTasks);
            m_masterCipherDataFromCalculator.insert(_cipherData.begin(), _cipherData.end());
            m_CipherDataFromCalculatorSubSeq.insert(seq);
            ECDH_MULTI_LOG(INFO) << LOG_KV(
                "Part1-C:Master Receive H(X)*A Size: ", m_CipherDataFromCalculatorSubSeq.size());
            if (m_CipherDataFromCalculatorSubSeq.size() == needSendTimes)
            {
                m_masterTaskPeersFinishedList.insert(_peerId);
            }
        }
        catch (std::exception& e)
        {
            ECDH_MULTI_LOG(ERROR) << LOG_DESC("appendMasterCipherDataFromCalculator Exception:")
                                  << boost::diagnostic_information(e);
        }
    }

    void appendMasterCipherDataFromPartner(std::string _peerId,
        std::vector<bcos::bytes>&& _cipherData, uint32_t seq, uint32_t needSendTimes)
    {
        try
        {
            bcos::WriteGuard lock(x_peerTasks);
            m_masterCipherDataFromPartner[_peerId].insert(
                m_masterCipherDataFromPartner[_peerId].end(), _cipherData.begin(),
                _cipherData.end());

            m_CipherDataFromPartnerSubSeq[_peerId].insert(seq);
            ECDH_MULTI_LOG(INFO) << LOG_KV(
                "Part1-P:Master Receive H(Y)*A Size: ", m_CipherDataFromPartnerSubSeq.size());
            if (m_CipherDataFromPartnerSubSeq[_peerId].size() == needSendTimes)
            {
                m_masterTaskPeersFinishedList.insert(_peerId);
            }
        }
        catch (std::exception& e)
        {
            ECDH_MULTI_LOG(ERROR) << LOG_DESC("appendMasterCipherDataFromPartner Exception:")
                                  << boost::diagnostic_information(e);
        }
    }

    std::set<std::string> const& masterTaskPeersFinishedList()
    {
        return m_masterTaskPeersFinishedList;
    }

    std::map<uint32_t, bcos::bytes> const& masterFinalIntersectionCipherData()
    {
        return m_masterFinalIntersectionCipherData;
    }

    void tryToGetCipherDataIntersection()
    {
        // partner intersection _masterCipherDataIntersectionFromPartner
        std::vector<bcos::bytes> _masterCipherDataIntersectionFromPartnerVector;
        std::map<bcos::bytes, uint32_t> cnt;
        uint32_t n = m_masterCipherDataFromPartner.size();
        for (auto& data : m_masterCipherDataFromPartner)
        {
            std::set _temp(data.second.begin(), data.second.end());
            for (auto& v : _temp)
            {
                cnt[v]++;
                if (cnt[v] == n)
                {
                    _masterCipherDataIntersectionFromPartnerVector.push_back(v);
                }
            }
        }

        // calculator intersection partners m_masterFinalIntersectionCipherData
        m_masterFinalIntersectionCipherData.clear();
        std::map<bcos::bytes, uint32_t> temp_out;
        for (auto& data : m_masterCipherDataFromCalculator)
        {
            // temp_out.insert(std::make_pair(data.second, data.first));
            temp_out.emplace(std::make_pair(data.second, data.first));
        }

        for (const auto& data : _masterCipherDataIntersectionFromPartnerVector)
        {
            auto it = temp_out.find(data);
            if (it != temp_out.end())
            {
                // m_masterFinalIntersectionCipherData.insert(std::make_pair(it->second,
                // it->first));
                m_masterFinalIntersectionCipherData.emplace(std::make_pair(it->second, it->first));
            }
        }
    }

private:
    std::string m_taskID;

    // store the cipher-data of the master
    std::map<uint32_t, bcos::bytes> m_masterFinalIntersectionCipherData;
    std::set<std::string> m_masterTaskPeersFinishedList;
    std::map<uint32_t, bcos::bytes> m_masterCipherDataFromCalculator;
    std::map<std::string, std::vector<bcos::bytes>> m_masterCipherDataFromPartner;
    // std::vector<bcos::bytes> m_masterCipherDataIntersectionFromPartner;
    std::set<uint32_t> m_CipherDataFromCalculatorSubSeq;
    std::map<std::string, std::set<uint32_t>> m_CipherDataFromPartnerSubSeq;
    bcos::SharedMutex x_peerTasks;
};

class CalculatorCipherDataCache
{
public:
    using Ptr = std::shared_ptr<CalculatorCipherDataCache>;
    CalculatorCipherDataCache() {}
    virtual ~CalculatorCipherDataCache()
    {
        m_CipherDataFromCalculatorSubSeq.clear();
        m_calculatorIntersectionSubSeq.clear();
        m_calculatorCipherDataSubSeq.clear();
        m_calculatorCipherData.clear();
        m_calculatorIntersectionCipherDataMap.clear();
        m_calculatorIntersectionCipherDataFinalMap.clear();
        std::set<uint32_t>().swap(m_CipherDataFromCalculatorSubSeq);
        std::set<uint32_t>().swap(m_calculatorIntersectionSubSeq);
        std::set<uint32_t>().swap(m_calculatorCipherDataSubSeq);
        std::vector<bcos::bytes>().swap(m_calculatorCipherData);
        std::map<uint32_t, bcos::bytes>().swap(m_calculatorIntersectionCipherDataMap);
        std::map<uint32_t, bcos::bytes>().swap(m_calculatorIntersectionCipherDataFinalMap);
        MallocExtension::instance()->ReleaseFreeMemory();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("the calculator cipher datacache destroyed")
                             << LOG_KV("taskID", m_taskID);
    }

    void tryToGetCipherDataIntersection()
    {
        m_calculatorIntersectionCipherDataFinalMap.clear();
        std::map<bcos::bytes, uint32_t> temp_out;
        for (auto& data : m_calculatorIntersectionCipherDataMap)
        {
            // temp_out.insert(std::make_pair(data.second, data.first));
            temp_out.emplace(std::make_pair(data.second, data.first));
        }

        for (const auto& data : m_calculatorCipherData)
        {
            auto it = temp_out.find(data);
            if (it != temp_out.end())
            {
                m_calculatorIntersectionCipherDataFinalMap.emplace(
                    std::make_pair(it->second, it->first));
            }
        }
    }

    bool setCalculatorCipherData(
        std::vector<bcos::bytes>&& _cipherData, uint32_t seq, uint32_t needSendTimes)
    {
        bcos::WriteGuard lock(x_setCalculatorCipherData);
        m_calculatorCipherData.insert(
            m_calculatorCipherData.end(), _cipherData.begin(), _cipherData.end());
        m_calculatorCipherDataSubSeq.insert(seq);
        return m_calculatorCipherDataSubSeq.size() == needSendTimes;
    }

    bool setCalculatorIntersectionCipherDataMap(
        std::map<uint32_t, bcos::bytes>&& _cipherData, uint32_t seq, uint32_t needSendTimes)
    {
        bcos::WriteGuard lock(x_setCalculatorIntersectionCipherData);
        m_calculatorIntersectionCipherDataMap.insert(_cipherData.begin(), _cipherData.end());
        m_calculatorIntersectionSubSeq.insert(seq);
        return m_calculatorIntersectionSubSeq.size() == needSendTimes;
    }

    std::map<uint32_t, bcos::bytes> const& calculatorIntersectionCipherDataFinalMap()
    {
        return m_calculatorIntersectionCipherDataFinalMap;
    }

private:
    std::string m_taskID;

    // store the cipher-data of the calculator
    std::set<uint32_t> m_CipherDataFromCalculatorSubSeq;
    std::set<uint32_t> m_calculatorIntersectionSubSeq;
    std::set<uint32_t> m_calculatorCipherDataSubSeq;
    std::vector<bcos::bytes> m_calculatorCipherData;
    std::map<uint32_t, bcos::bytes> m_calculatorIntersectionCipherDataMap;
    std::map<uint32_t, bcos::bytes> m_calculatorIntersectionCipherDataFinalMap;

    mutable boost::shared_mutex x_setCalculatorCipherData;
    mutable boost::shared_mutex x_setCalculatorIntersectionCipherData;
};
}  // namespace ppc::psi