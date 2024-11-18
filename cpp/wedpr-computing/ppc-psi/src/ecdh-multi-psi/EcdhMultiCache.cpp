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
using namespace ppc::protocol;
using namespace bcos;

void MasterCache::addCalculatorCipher(std::string _peerId, std::vector<bcos::bytes>&& _cipherData,
    std::vector<long> const& dataIndex, uint32_t seq, uint32_t dataBatchCount)
{
    auto peerIndex = getPeerIndex(_peerId);
    if (peerIndex == -1)
    {
        ECDH_MULTI_LOG(WARNING) << LOG_DESC("Invalid calculator") << LOG_KV("peer", _peerId);
        return;
    }
    bcos::Guard l(m_mutex);
    m_calculatorCipherSeqs.insert(seq);
    if (dataBatchCount)
    {
        m_calculatorDataBatchCount = dataBatchCount;
    }
    uint64_t i = 0;
    for (auto&& it : _cipherData)
    {
        updateMasterDataRef(peerIndex, std::move(it), dataIndex[i]);
        i++;
    }
    // try to merge the
    if (m_calculatorDataBatchCount > 0 &&
        m_calculatorCipherSeqs.size() == m_calculatorDataBatchCount)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("The master receive all cipher data from the calculator")
                             << LOG_KV("seq", seq) << LOG_KV("calculator", _peerId)
                             << LOG_KV("masterData", m_masterDataRef.size()) << printCacheState();
        m_finishedPartners.insert(_peerId);
        // try to merge
        mergeMasterCipher(_peerId, peerIndex);
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC(
                                "addCalculatorCipher: master receive cipher data from calculator")
                         << LOG_KV("calculator", _peerId) << printCacheState() << LOG_KV("seq", seq)
                         << LOG_KV("receivedSize", _cipherData.size())
                         << LOG_KV("masterData", m_masterDataRef.size())
                         << LOG_KV("dataBatchCount", m_calculatorDataBatchCount);
    // release the cipherData
    _cipherData.clear();
    std::vector<bcos::bytes>().swap(_cipherData);
    MallocExtension::instance()->ReleaseFreeMemory();
}

void MasterCache::updateMasterDataRef(
    unsigned short _peerIndex, bcos::bytes&& data, int32_t dataIndex)
{
    // not merged case
    if (!m_peerMerged)
    {
        // new data case
        if (!m_masterDataRef.count(data))
        {
            MasterCipherRef ref;
            ref.refInfo |= (1 << _peerIndex);
            ref.updateDataIndex(dataIndex);
            m_masterDataRef.insert(std::make_pair(std::move(data), ref));
            return;
        }
        // existed data case
        m_masterDataRef[data].refInfo |= (1 << _peerIndex);
        m_masterDataRef[data].updateDataIndex(dataIndex);
        return;
    }

    // merged case, only record the intersection case, increase the refCount
    if (m_masterDataRef.count(data))
    {
        m_masterDataRef[data].refCount += 1;
        m_masterDataRef[data].updateDataIndex(dataIndex);
    }
}


void MasterCache::addPartnerCipher(std::string _peerId, std::vector<bcos::bytes>&& _cipherData,
    uint32_t seq, uint32_t parternerDataCount)
{
    auto peerIndex = getPeerIndex(_peerId);
    if (peerIndex == -1)
    {
        ECDH_MULTI_LOG(WARNING) << LOG_DESC("Invalid peerId") << LOG_KV("peer", _peerId);
        return;
    }
    bcos::Guard lock(m_mutex);
    // record the data-ref-count
    for (auto&& data : _cipherData)
    {
        updateMasterDataRef(peerIndex, std::move(data), -1);
    }
    m_partnerCipherSeqs[_peerId].insert(seq);
    ECDH_MULTI_LOG(INFO) << LOG_DESC("addPartnerCipher") << LOG_KV("partner", _peerId)
                         << LOG_KV("seq", seq)
                         << LOG_KV("receivedBatch", m_partnerCipherSeqs.at(_peerId).size())
                         << LOG_KV("cipherDataSize", _cipherData.size())
                         << LOG_KV("masterDataSize", m_masterDataRef.size())
                         << LOG_KV("parternerDataCount", parternerDataCount) << printCacheState();
    _cipherData.clear();
    std::vector<bcos::bytes>().swap(_cipherData);
    MallocExtension::instance()->ReleaseFreeMemory();
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
        // merge when find the send-finished peer
        mergeMasterCipher(_peerId, peerIndex);
    }
}

void MasterCache::mergeMasterCipher(std::string const& peerId, unsigned short peerIndex)
{
    if (m_peerMerged)
    {
        return;
    }
    // no need to merge when partnerCount is 1
    if (m_peerCount == 1)
    {
        return;
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC("Receive whole data from peer, mergeMasterCipher")
                         << LOG_KV("distinct-masterDataSize-before-merge", m_masterDataRef.size())
                         << LOG_KV("finishedPeer", peerId) << LOG_KV("partnerCount", m_peerCount);
    auto startT = utcSteadyTime();
    for (auto it = m_masterDataRef.begin(); it != m_masterDataRef.end();)
    {
        // not has intersect-element with the finished peer
        if (!(it->second.refInfo & (1 << peerIndex)))
        {
            it = m_masterDataRef.erase(it);
            continue;
        }
        // reset the refCount
        it->second.resetRefCount();
        it++;
    }
    m_peerMerged = true;
    // release the free memory after merged
    MallocExtension::instance()->ReleaseFreeMemory();
    ECDH_MULTI_LOG(INFO) << LOG_DESC("mergeMasterCipher finished")
                         << LOG_KV("distinct-masterDataSize-after-merge", m_masterDataRef.size())
                         << LOG_KV("finishedPeer", peerId) << LOG_KV("peerIndex", peerIndex)
                         << LOG_KV("timecost", (utcSteadyTime() - startT));
}

// get the cipher-data intersection: h(x)^a && h(Y)^a
bool MasterCache::tryToIntersection()
{
    if (!shouldIntersection())
    {
        return false;
    }
    m_cacheState = CacheState::IntersectionProgressing;

    ECDH_MULTI_LOG(INFO) << LOG_DESC("* tryToIntersection ") << printCacheState()
                         << LOG_KV("* masterData", m_masterDataRef.size());
    auto startT = utcSteadyTime();
    // iterator the masterDataRef to obtain intersection
    for (auto&& it : m_masterDataRef)
    {
        if (!m_masterDataRef.count(it.first))
        {
            continue;
        }
        if (m_masterDataRef.at(it.first).refCount != m_peerCount)
        {
            continue;
        }
        if (m_masterDataRef.at(it.first).dataIndex == -1)
        {
            continue;
        }
        // intersection case
        m_intersecCipher.emplace_back(std::move(it.first));
        m_intersecCipherIndex.emplace_back(it.second.dataIndex);
    }
    releaseCache();
    m_cacheState = CacheState::Intersectioned;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("* tryToIntersection success") << printCacheState()
                         << LOG_KV("* intersectionSize", m_intersecCipher.size())
                         << LOG_KV("* timecost", (utcSteadyTime() - startT));
    return true;
}

void MasterCache::encryptAndSendIntersection(uint64_t dataBatchIdx, bcos::bytes const& randomKey,
    std::map<std::string, PartyResource::Ptr> const& calculators)
{
    auto startT = utcSteadyTime();
    auto message = m_config->psiMsgFactory()->createPSIMessage(
        uint32_t(EcdhMultiPSIMessageType::SEND_ENCRYPTED_INTERSECTION_SET_TO_CALCULATOR));
    message->setFrom(m_taskState->task()->selfParty()->id());
    size_t startIndex = dataBatchIdx * m_config->dataBatchSize();
    size_t endIndex = std::min(startIndex + m_config->dataBatchSize(), m_intersecCipher.size());
    ECDH_MULTI_LOG(INFO) << LOG_DESC("encryptAndSendIntersection")
                         << LOG_KV("cipherCount", m_intersecCipher.size())
                         << LOG_KV("dataBatchIdx", dataBatchIdx) << LOG_KV("start", startIndex)
                         << LOG_KV("end", endIndex) << printCacheState();
    message->resizeData(endIndex - startIndex);
    if (endIndex >= m_intersecCipher.size())
    {
        message->setDataBatchCount(dataBatchIdx + 1);
    }
    tbb::parallel_for(tbb::blocked_range<size_t>(startIndex, endIndex), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            auto cipherValue = m_config->eccCrypto()->ecMultiply(m_intersecCipher[i], randomKey);
            message->setDataPair(
                i - startIndex, m_intersecCipherIndex[i - startIndex], cipherValue);
        }
    });
    auto taskID = m_taskState->task()->id();
    auto self = weak_from_this();
    for (auto const& calculator : calculators)
    {
        m_config->generateAndSendPPCMessage(
            calculator.first, taskID, message,
            [self, calculator](bcos::Error::Ptr&& _error) {
                if (!_error || _error->errorCode() == 0)
                {
                    return;
                }
                auto cache = self.lock();
                if (!cache)
                {
                    return;
                }
                ECDH_MULTI_LOG(WARNING)
                    << LOG_DESC("encryptAndSendIntersection: send message to calcultor failed")
                    << LOG_KV("task", cache->m_taskState->task()->id())
                    << LOG_KV("calculator", calculator.first) << LOG_KV("code", _error->errorCode())
                    << LOG_KV("msg", _error->errorMessage());
                cache->m_taskState->onTaskException(_error->errorMessage());
            },
            dataBatchIdx);
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC("encryptAndSendIntersection success")
                         << LOG_KV("cipherCount", m_intersecCipher.size())
                         << LOG_KV("dataBatchIdx", dataBatchIdx)
                         << LOG_KV("timecost", utcSteadyTime() - startT) << printCacheState();
}

void MasterCache::encryptIntersection(
    bcos::bytes const& randomKey, std::map<std::string, PartyResource::Ptr> const& calculators)
{
    auto dataBatchCount =
        std::max((uint64_t)(m_intersecCipher.size() + m_config->dataBatchSize() - 1) /
                     (uint64_t)m_config->dataBatchSize(),
            (uint64_t)1);
    ECDH_MULTI_LOG(INFO) << LOG_DESC("encryptIntersection")
                         << LOG_KV("cipherCount", m_intersecCipher.size())
                         << LOG_KV("dataBatchCount", dataBatchCount) << printCacheState();
    auto startT = utcSteadyTime();
    for (uint64_t i = 0; i < dataBatchCount; i++)
    {
        encryptAndSendIntersection(i, randomKey, calculators);
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC("encryptIntersection success")
                         << LOG_KV("timecost", (utcSteadyTime() - startT))
                         << LOG_KV("cipherCount", m_intersecCipher.size()) << printCacheState();
    // Note: release the m_intersecCipher, make share it not been used after released
    releaseIntersection();
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
    ECDH_MULTI_LOG(INFO) << LOG_DESC("* tryToFinalize: compute intersection")
                         << LOG_KV("* cipherRef", m_cipherRef.size()) << printCacheState();
    m_cacheState = CacheState::Finalizing;
    // find the intersection
    for (auto const& it : m_cipherRef)
    {
        if (it.second.refCount < 2)
        {
            continue;
        }
        if (it.second.plainDataIndex > 0)
        {
            m_intersectionResult.emplace_back(getPlainDataByIndex(it.second.plainDataIndex));
        }
        // means the header field, swap with the first element
        if (it.second.plainDataIndex == 0)
        {
            m_intersectionResult.emplace_back(m_intersectionResult[0]);
            m_intersectionResult[0] = getPlainDataByIndex(it.second.plainDataIndex);
        }
    }
    m_cacheState = CacheState::Finalized;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("* tryToFinalize:  compute intersection success")
                         << printCacheState() << LOG_KV("* cipherRef", m_cipherRef.size())
                         << LOG_KV("* intersectionSize", m_intersectionResult.size())
                         << LOG_KV("* timecost", (utcSteadyTime() - startT));

    releaseCipherCache();
    ECDH_MULTI_LOG(INFO) << LOG_DESC("* tryToFinalize: syncIntersections") << printCacheState();
    m_cacheState = CacheState::Syncing;
    syncIntersections();
    m_cacheState = CacheState::Synced;

    m_cacheState = CacheState::StoreProgressing;
    m_taskState->storePSIResult(m_config->dataResourceLoader(), m_intersectionResult);
    m_cacheState = CacheState::Stored;
    ECDH_MULTI_LOG(INFO) << LOG_DESC("* tryToFinalize: syncIntersections and store success")
                         << printCacheState();
    return true;
}

void CalculatorCache::syncIntersections()
{
    ECDH_MULTI_LOG(INFO) << LOG_DESC("*** syncIntersections **") << printCacheState();
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
            // Note: sync task failed will not change the task status
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
        // Note: sync task failed will not change the task status
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

void CalculatorCache::updateCipherRef(bcos::bytes&& data, int32_t index)
{
    // case that receive at least one completed data, only record the intersection data
    if (m_receiveAllMasterCipher || m_receiveAllIntersection)
    {
        if (!m_cipherRef.count(data))
        {
            return;
        }
    }
    // new data case
    if (!m_cipherRef.count(data))
    {
        CipherRefDetail cipherRef;
        cipherRef.refCount = 1;
        cipherRef.updatePlainIndex(index);
        m_cipherRef.insert(std::make_pair(std::move(data), std::move(cipherRef)));
        return;
    }
    // existed data case
    m_cipherRef[data].refCount += 1;
    m_cipherRef[data].updatePlainIndex(index);
}


bool CalculatorCache::appendMasterCipher(
    std::vector<bcos::bytes>&& _cipherData, uint32_t seq, uint32_t dataBatchSize)
{
    bcos::Guard lock(m_mutex);
    m_receivedMasterCipher.insert(seq);
    for (auto&& it : _cipherData)
    {
        updateCipherRef(std::move(it), -1);
    }
    if (m_masterDataBatchSize == 0 && dataBatchSize > 0)
    {
        m_masterDataBatchSize = dataBatchSize;
    }
    if (!m_receiveAllMasterCipher && m_receivedMasterCipher.size() == m_masterDataBatchSize)
    {
        m_receiveAllMasterCipher = true;
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC("appendMasterCipher") << LOG_KV("dataSize", _cipherData.size())
                         << LOG_KV("cipherRefSize", m_cipherRef.size()) << printCacheState();
    // release the cipherData
    _cipherData.clear();
    std::vector<bcos::bytes>().swap(_cipherData);
    MallocExtension::instance()->ReleaseFreeMemory();
    return m_receiveAllMasterCipher;
}

void CalculatorCache::addIntersectionCipher(std::vector<bcos::bytes>&& _cipherData,
    std::vector<long> const& dataIndex, uint32_t seq, uint64_t dataBatchCount)
{
    auto startT = utcSteadyTime();
    ECDH_MULTI_LOG(INFO) << LOG_DESC("addIntersectionCipher") << LOG_KV("seq", seq)
                         << LOG_KV("dataSize", _cipherData.size())
                         << LOG_KV("cipherRefSize", m_cipherRef.size()) << printCacheState()
                         << LOG_KV("dataBatchCount", dataBatchCount);
    bcos::Guard lock(m_mutex);
    uint64_t i = 0;
    for (auto&& it : _cipherData)
    {
        updateCipherRef(std::move(it), dataIndex[i]);
        i++;
    }
    m_receivedIntersections.insert(seq);
    if (dataBatchCount > 0)
    {
        m_intersectionBatchCount = dataBatchCount;
    }
    if (m_intersectionBatchCount > 0 && m_receivedIntersections.size() >= m_intersectionBatchCount)
    {
        m_receiveAllIntersection = true;
    }
    ECDH_MULTI_LOG(INFO) << LOG_DESC("addIntersectionCipher success")
                         << LOG_KV("timecost", utcSteadyTime() - startT) << LOG_KV("seq", seq)
                         << LOG_KV("dataSize", _cipherData.size())
                         << LOG_KV("receiveAll", m_receiveAllIntersection)
                         << LOG_KV("cipherRefSize", m_cipherRef.size()) << printCacheState();
    // release the cipherData
    _cipherData.clear();
    std::vector<bcos::bytes>().swap(_cipherData);
    MallocExtension::instance()->ReleaseFreeMemory();
}