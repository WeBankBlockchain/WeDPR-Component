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
#include "EcdhMultiCache.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

using namespace ppc::psi;
using namespace bcos;

void MasterCache::addCalculatorCipher(std::string _peerId,
    std::map<uint32_t, bcos::bytes>&& _cipherData, uint32_t seq, uint32_t dataBatchCount)
{
    bcos::WriteGuard l(x_calculatorCipher);
    m_calculatorCipher.insert(_cipherData.begin(), _cipherData.end());
    m_calculatorCipherSeqs.insert(seq);
    if (dataBatchCount)
    {
        m_calculatorDataBatchCount = dataBatchCount;
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC(
                                "addCalculatorCipher: master receive cipher data from calculator")
                         << LOG_KV("calculator", _peerId) << printCacheState()
                         << LOG_KV("receivedSize", m_calculatorCipherSeqs.size())
                         << LOG_KV("calculatorCipherSize", m_calculatorCipher.size())
                         << LOG_KV("dataBatchCount", m_calculatorDataBatchCount);
    if (m_calculatorDataBatchCount > 0 &&
        m_calculatorCipherSeqs.size() == m_calculatorDataBatchCount)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("The master receive all cipher data from the calculator")
                             << LOG_KV("calculatorId", _peerId) << printCacheState();
        m_finishedPartners.insert(_peerId);
    }
}

void MasterCache::addPartnerCipher(std::string _peerId, std::vector<bcos::bytes>&& _cipherData,
    uint32_t seq, uint32_t parternerDataCount)
{
    bcos::WriteGuard lock(x_partnerToCipher);
    if (!m_partnerToCipher.count(_peerId))
    {
        m_partnerToCipher.insert(std::make_pair(_peerId, std::set<bcos::bytes>()));
    }
    m_partnerToCipher[_peerId].insert(_cipherData.begin(), _cipherData.end());
    m_partnerCipherSeqs[_peerId].insert(seq);
    ECDH_MULTI_LOG(INFO) << LOG_DESC("addPartnerCipher") << LOG_KV("partner", _peerId)
                         << LOG_KV("seqSize", m_partnerCipherSeqs.at(_peerId).size())
                         << LOG_KV("cipherDataSize", _cipherData.size()) << printCacheState();
    if (parternerDataCount > 0)
    {
        m_parternerDataCount.insert(std::make_pair(_peerId, parternerDataCount));
    }
    if (!m_parternerDataCount.count(_peerId))
    {
        return;
    }
    auto expectedCount = m_parternerDataCount.at(_peerId);
    if (m_partnerCipherSeqs[_peerId].size() == expectedCount)
    {
        m_finishedPartners.insert(_peerId);
    }
}

// get the cipher-data intersection: h(x)^a && h(Y)^a
bool MasterCache::tryToIntersection()
{
    if (!shouldIntersection())
    {
        return false;
    }
    m_cacheState = CacheState::IntersectionProgressing;

    ECDH_MULTI_LOG(INFO) << LOG_DESC("tryToIntersection ") << printCacheState()
                         << LOG_KV("calculatorCipher", m_calculatorCipher.size());
    auto startT = utcSteadyTime();
    // iterator the calculator cipher to obtain intersection
    for (auto&& it : m_calculatorCipher)
    {
        bool insersected = true;
        for (auto const& partnerIter : m_partnerToCipher)
        {
            // not the intersection case
            if (!partnerIter.second.count(it.second))
            {
                insersected = false;
                break;
            }
        }
        if (insersected)
        {
            m_intersecCipher.emplace_back(std::move(it.second));
            m_intersecCipherIndex.emplace_back(it.first);
        }
    }
    m_cacheState = CacheState::Intersectioned;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("tryToIntersection success") << printCacheState()
                         << LOG_KV("timecost", (utcSteadyTime() - startT));
    return true;
}

std::vector<std::pair<uint64_t, bcos::bytes>> MasterCache::encryptIntersection(
    bcos::bytes const& randomKey)
{
    std::vector<std::pair<uint64_t, bcos::bytes>> cipherData(m_intersecCipher.size());
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0U, m_intersecCipher.size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto cipherValue =
                    m_config->eccCrypto()->ecMultiply(m_intersecCipher[i], randomKey);
                cipherData[i] = std::make_pair(m_intersecCipherIndex[i], cipherValue);
            }
        });
    // Note: release the m_intersecCipher, make share it not been used after released
    releaseIntersecCipher();
    return cipherData;
}

bcos::bytes CalculatorCache::getPlainDataByIndex(uint64_t index)
{
    uint64_t startIndex = 0;
    uint64_t endIndex = 0;
    for (auto const& it : m_plainData)
    {
        endIndex += it->size();
        if (index >= startIndex && index < endIndex)
        {
            return it->getBytes((index - startIndex));
        }
        startIndex += it->size();
    }
    return bcos::bytes();
}

bool CalculatorCache::tryToFinalize()
{
    if (!shouldFinalize())
    {
        return false;
    }
    auto startT = utcSteadyTime();
    ECDH_MULTI_LOG(INFO) << LOG_DESC("tryToFinalize: compute intersection") << printCacheState();
    m_cacheState = CacheState::Finalizing;
    // find the intersection
    for (auto const& it : m_intersectionCipher)
    {
        if (m_masterCipher.count(it.second))
        {
            auto ret = getPlainDataByIndex(it.first);
            if (ret.size() > 0)
            {
                m_intersectionResult.emplace_back(ret);
            }
        }
    }
    m_cacheState = CacheState::Finalized;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("tryToFinalize:  compute intersection success")
                         << printCacheState()
                         << LOG_KV("intersectionSize", m_intersectionResult.size())
                         << LOG_KV("timecost", (utcSteadyTime() - startT));

    ECDH_MULTI_LOG(INFO) << LOG_DESC("tryToFinalize: syncIntersections") << printCacheState();
    m_cacheState = CacheState::Syncing;
    syncIntersections();
    m_cacheState = CacheState::Synced;

    m_cacheState = CacheState::StoreProgressing;
    m_taskState->storePSIResult(m_config->dataResourceLoader(), m_intersectionResult);
    m_cacheState = CacheState::Stored;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("tryToFinalize: syncIntersections and store success")
                         << printCacheState();
    return true;
}

void CalculatorCache::syncIntersections()
{
    ECDH_MULTI_LOG(INFO) << LOG_DESC("syncIntersections") << printCacheState();
    auto peers = m_taskState->task()->getAllPeerParties();
    auto taskID = m_taskState->task()->id();
    // notify task result
    if (!m_syncResult)
    {
        auto message = m_config->psiMsgFactory()->createPSIMessage(
            uint32_t(EcdhMultiPSIMessageType::SYNC_FINAL_RESULT_TO_ALL));
        message->setFrom(m_taskState->task()->selfParty()->id());
        message->setVersion(-1);
        for (auto& peer : peers)
        {
            m_config->generateAndSendPPCMessage(
                peer.first, taskID, message,
                [taskID, peer](bcos::Error::Ptr&& _error) {
                    if (_error && _error->errorCode() != 0)
                    {
                        ECDH_MULTI_LOG(WARNING)
                            << LOG_DESC("sync task result to peer failed")
                            << LOG_KV("peer", peer.first) << LOG_KV("taskID", taskID)
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        return;
                    }
                },
                0);
        }
        return;
    }
    // sync intersectionResult to all peers
    auto message = m_config->psiMsgFactory()->createPSIMessage(
        uint32_t(EcdhMultiPSIMessageType::SYNC_FINAL_RESULT_TO_ALL));
    message->setData(m_intersectionResult);
    message->setFrom(m_taskState->task()->selfParty()->id());
    message->setVersion(0);
    for (auto& peer : peers)
    {
        m_config->generateAndSendPPCMessage(
            peer.first, taskID, message,
            [taskID, peer](bcos::Error::Ptr&& _error) {
                if (_error && _error->errorCode() != 0)
                {
                    ECDH_MULTI_LOG(WARNING)
                        << LOG_DESC("sync psi result to peer failed") << LOG_KV("peer", peer.first)
                        << LOG_KV("taskID", taskID) << LOG_KV("code", _error->errorCode())
                        << LOG_KV("msg", _error->errorMessage());
                    return;
                }
            },
            0);
    }
}


bool CalculatorCache::appendMasterCipher(
    std::vector<bcos::bytes>&& _cipherData, uint32_t seq, uint32_t dataBatchSize)
{
    bcos::WriteGuard lock(x_masterCipher);
    m_masterCipher.insert(_cipherData.begin(), _cipherData.end());
    m_receivedMasterCipher.insert(seq);
    if (m_masterDataBatchSize == 0 && dataBatchSize > 0)
    {
        m_masterDataBatchSize = dataBatchSize;
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC("appendMasterCipher") << LOG_KV("dataSize", _cipherData.size())
                         << printCacheState();
    return m_receivedMasterCipher.size() == m_masterDataBatchSize;
}

void CalculatorCache::setIntersectionCipher(std::map<uint32_t, bcos::bytes>&& _cipherData)
{
    bcos::WriteGuard lock(x_intersectionCipher);
    m_intersectionCipher = std::move(_cipherData);
    m_receiveIntersection = true;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("setIntersectionCipher")
                         << LOG_KV("dataSize", _cipherData.size()) << printCacheState();
}