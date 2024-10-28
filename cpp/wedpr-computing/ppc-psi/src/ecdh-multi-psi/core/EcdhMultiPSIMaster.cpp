#include "EcdhMultiPSIMaster.h"
#include "ppc-psi/src/ecdh-multi-psi/Common.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>


using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace bcos;

EcdhMultiPSIMaster::EcdhMultiPSIMaster(EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState)
  : m_config(std::move(_config)), m_taskState(std::move(_taskState))
{
    auto task = m_taskState->task();
    auto receivers = task->getReceiverLists();
    m_taskID = task->id();
    m_masterCipherDataCache = std::make_shared<MasterCipherDataCache>();
    m_final_counts[m_taskID] = 0;
    m_syncResult = (task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                    m_config->selfParty()) != receivers.end());
}

void EcdhMultiPSIMaster::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    InitAsyncTask(_task);
    ECDH_MULTI_LOG(INFO) << LOG_DESC("Master asyncStartRunTask") << printTaskInfo(_task);
    auto B = m_config->eccCrypto()->generateRandomScalar();
    m_randomB = std::make_shared<bcos::bytes>(B);
}

void EcdhMultiPSIMaster::InitAsyncTask(ppc::protocol::Task::ConstPtr _task)
{
    // Init all Roles from all Peers
    auto peerParties = _task->getAllPeerParties();
    for (auto& party : peerParties)
    {
        auto partyId = party.first;
        auto partySource = party.second;
        if (partySource->partyIndex() == uint16_t(PartiesType::Calculator))
        {
            m_calculatorParties[partyId] = partySource;
        }
        if (partySource->partyIndex() == uint16_t(PartiesType::Partner))
        {
            m_partnerParties[partyId] = partySource;
        }
        if (partySource->partyIndex() == uint16_t(PartiesType::Master))
        {
            m_masterParties[partyId] = partySource;
        }
    }
}

// Part1-C,2-C: Master -> Calculator [H(X)*A ∩ H(Y)*A]*B
void EcdhMultiPSIMaster::onHandlerIntersectEncryptSetFromCalculator(PSIMessageInterface::Ptr _msg)
{
    try
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Part1-C: Master Receive H(X)*A ")
                             << LOG_KV(" PSIMessageFace Size: ", _msg->takeDataMap().size())
                             << LOG_KV(" PSIMessageFace SEQ: ", _msg->seq())
                             << LOG_KV(" PSIMessageFace dataBatchCount: ", _msg->dataBatchCount());
        WriteGuard lock(x_appendMasterCipherDataFromCalculator);
        m_masterCipherDataCache->appendMasterCipherDataFromCalculator(
            _msg->from(), _msg->takeDataMap(), _msg->seq(), _msg->dataBatchCount());
        if (loadCipherDataFinished())
        {
            m_masterCipherDataCache->tryToGetCipherDataIntersection();
            auto cipherMaps = m_masterCipherDataCache->masterFinalIntersectionCipherData();
            ECDH_MULTI_LOG(INFO) << LOG_DESC("Part2-C:Master [H(X)*A ∩ H(Y)*A]")
                                 << LOG_KV(
                                        " Received Cipher Set Success Size: ", cipherMaps.size());
            tbb::concurrent_map<uint32_t, bcos::bytes> encryptedInterHashMap;
            tbb::parallel_for_each(cipherMaps.begin(), cipherMaps.end(), [&](auto const& _pair) {
                auto index = _pair.first;
                auto value = _pair.second;
                if (value.data())
                {
                    auto hashSet = m_config->eccCrypto()->ecMultiply(value, *m_randomB);
                    // encryptedInterHashMap.insert(std::make_pair(index, hashSet));
                    encryptedInterHashMap.emplace(std::make_pair(index, hashSet));
                }
            });

            auto inputSize = encryptedInterHashMap.size();
            auto batchSize = m_config->dataBatchSize();
            auto needSendTimes = 0;
            // send counts
            if (inputSize % batchSize == 0)
            {
                needSendTimes = inputSize / batchSize;
            }
            else
            {
                needSendTimes = inputSize / batchSize + 1;
            }
            uint32_t readStart = 0, readCount = 0;
            while (readStart < inputSize)
            {
                if (inputSize < batchSize)
                {
                    readCount = batchSize;
                }
                else if (readCount + batchSize < inputSize)
                {
                    readCount += batchSize;
                }
                else
                {
                    readCount = inputSize;
                }
                std::map<uint32_t, bcos::bytes> cHashMap;
                ConcurrentSTLToCommon(encryptedInterHashMap, readStart, readCount, cHashMap);

                for (auto& calcultor : m_calculatorParties)
                {
                    ECDH_MULTI_LOG(INFO)
                        << LOG_KV(
                               "Part2-C:Master Send the [H(X)*A ∩ H(Y)*A]*B success to "
                               "Calculator: ",
                               calcultor.first)
                        << LOG_KV("EncryptSet [H(X)*A ∩ H(Y)*A]*B Size: ", cHashMap.size());
                    auto message = m_config->psiMsgFactory()->createPSIMessage(uint32_t(
                        EcdhMultiPSIMessageType::SEND_ENCRYPTED_INTERSECTION_SET_TO_CALCULATOR));
                    message->setDataMap(std::move(cHashMap));
                    message->setFrom(m_taskState->task()->selfParty()->id());
                    message->setDataBatchCount(needSendTimes);
                    m_config->generateAndSendPPCMessage(
                        calcultor.first, m_taskID, message,
                        [self = weak_from_this()](bcos::Error::Ptr&& _error) {
                            if (!_error)
                            {
                                return;
                            }
                            auto psi = self.lock();
                            if (!psi)
                            {
                                return;
                            }
                        },
                        readCount);
                }
                readStart += batchSize;
            }
        }
    }
    catch (std::exception& e)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Exception in onHandlerIntersectEncryptSetFromCalculator:")
                             << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

// Part1-P,2-P: Partner -> Master [H(X)*A ∩ H(Y)*A]*B
void EcdhMultiPSIMaster::onHandlerIntersectEncryptSetFromPartner(PSIMessageInterface::Ptr _msg)
{
    try
    {
        WriteGuard lock(x_appendMasterCipherDataFromPartner);
        m_masterCipherDataCache->appendMasterCipherDataFromPartner(
            _msg->from(), _msg->takeData(), _msg->seq(), _msg->dataBatchCount());
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Part1-P: Master Receive H(Y)*A")
                             << LOG_KV(" PSIMessageFace Size: ", _msg->takeData().size())
                             << LOG_KV(" PSIMessageFace SEQ: ", _msg->seq())
                             << LOG_KV(" PSIMessageFace dataBatchCount: ", _msg->dataBatchCount());

        if (loadCipherDataFinished())
        {
            m_masterCipherDataCache->tryToGetCipherDataIntersection();
            auto cipherMaps = m_masterCipherDataCache->masterFinalIntersectionCipherData();
            ECDH_MULTI_LOG(INFO) << LOG_DESC("Part2-P:Master Receive [H(X)*A ∩ H(Y)*A]")
                                 << LOG_KV(" Receive [H(X)*A ∩ H(Y)*A] Success Size: ",
                                        cipherMaps.size());
            tbb::concurrent_map<uint32_t, bcos::bytes> encryptedInterHashMap;

            tbb::parallel_for_each(cipherMaps.begin(), cipherMaps.end(), [&](auto const& _pair) {
                auto index = _pair.first;
                auto value = _pair.second;
                if (value.data())
                {
                    auto hashSet = m_config->eccCrypto()->ecMultiply(value, *m_randomB);
                    // encryptedInterHashMap.insert(std::make_pair(index, hashSet));
                    encryptedInterHashMap.emplace(std::make_pair(index, hashSet));
                }
            });

            auto inputSize = encryptedInterHashMap.size();
            auto batchSize = m_config->dataBatchSize();
            auto needSendTimes = 0;
            // send counts
            if (inputSize == 0)
            {
                needSendTimes = 1;
                for (auto& calcultor : m_calculatorParties)
                {
                    std::map<uint32_t, bcos::bytes> cHashMap;
                    ECDH_MULTI_LOG(INFO)
                        << LOG_KV(
                               "Part2-P: Master Send the [H(X)*A ∩ H(Y)*A]*B success to "
                               "Calculator: ",
                               calcultor.first)
                        << LOG_KV("EncryptSet [H(X)*A ∩ H(Y)*A]*B Size: ", 0);
                    auto message = m_config->psiMsgFactory()->createPSIMessage(uint32_t(
                        EcdhMultiPSIMessageType::SEND_ENCRYPTED_INTERSECTION_SET_TO_CALCULATOR));
                    message->setDataMap(std::move(cHashMap));
                    message->setFrom(m_taskState->task()->selfParty()->id());
                    message->setDataBatchCount(needSendTimes);
                    m_config->generateAndSendPPCMessage(
                        calcultor.first, m_taskID, message,
                        [self = weak_from_this()](bcos::Error::Ptr&& _error) {
                            if (!_error)
                            {
                                return;
                            }
                            auto psi = self.lock();
                            if (!psi)
                            {
                                return;
                            }
                        },
                        0);
                }
                return;
            }
            else if (inputSize % batchSize == 0)
            {
                needSendTimes = inputSize / batchSize;
            }
            else
            {
                needSendTimes = inputSize / batchSize + 1;
            }
            uint32_t readStart = 0, readCount = 0;
            while (readCount < inputSize)
            {
                if (inputSize < batchSize)
                {
                    readCount = batchSize;
                }
                else if (readCount + batchSize < inputSize)
                {
                    readCount += batchSize;
                }
                else
                {
                    readCount = inputSize;
                }
                std::map<uint32_t, bcos::bytes> cHashMap;
                ConcurrentSTLToCommon(encryptedInterHashMap, readStart, readCount, cHashMap);

                for (auto& calcultor : m_calculatorParties)
                {
                    ECDH_MULTI_LOG(INFO)
                        << LOG_KV(
                               "Part2-P: Master Send the [H(X)*A ∩ H(Y)*A]*B success to "
                               "Calculator: ",
                               calcultor.first)
                        << LOG_KV("EncryptSet [H(X)*A ∩ H(Y)*A]*B Size: ", cHashMap.size());
                    auto message = m_config->psiMsgFactory()->createPSIMessage(uint32_t(
                        EcdhMultiPSIMessageType::SEND_ENCRYPTED_INTERSECTION_SET_TO_CALCULATOR));
                    message->setDataMap(std::move(cHashMap));
                    message->setFrom(m_taskState->task()->selfParty()->id());
                    message->setDataBatchCount(needSendTimes);
                    m_config->generateAndSendPPCMessage(
                        calcultor.first, m_taskID, message,
                        [self = weak_from_this()](bcos::Error::Ptr&& _error) {
                            if (!_error)
                            {
                                return;
                            }
                            auto psi = self.lock();
                            if (!psi)
                            {
                                return;
                            }
                        },
                        readCount);
                }
                readStart += batchSize;
            }
        }
    }
    catch (std::exception& e)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Exception in onHandlerIntersectEncryptSetFromPartner:")
                             << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

// Part3: Master -> Calculator H(Z)*B
void EcdhMultiPSIMaster::onHandlerEncryptIntersectionSetFromCalculatorToMaster(
    PSIMessageInterface::Ptr _msg)
{
    try
    {
        m_originInputs = m_taskState->loadAllData();
        if (!m_originInputs || m_originInputs->size() == 0)
        {
            BOOST_THROW_EXCEPTION(ECDHMULTIException() << bcos::errinfo_comment("data is empty"));
        }
        auto inputSize = m_originInputs->size();
        auto batchSize = m_config->dataBatchSize();
        auto needSendTimes = 0;
        // send counts
        if (inputSize % batchSize == 0)
        {
            needSendTimes = inputSize / batchSize;
        }
        else
        {
            needSendTimes = inputSize / batchSize + 1;
        }
        ECDH_MULTI_LOG(INFO) << LOG_KV("Part3: Master computeAndEncryptSet RandomB: ",
                                    m_randomB->data())
                             << LOG_KV(" Load All Data Size: ", inputSize);
        auto hash = m_config->hash();
        uint32_t readStart = 0, readCount = 0;
        std::vector<bcos::bytes> encryptedHashSet;
        encryptedHashSet.reserve(inputSize);
        encryptedHashSet.resize(inputSize);
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto data = m_originInputs->getBytes(i);
                auto hashData = hash->hash(bcos::bytesConstRef(data.data(), data.size()));
                auto point = m_config->eccCrypto()->hashToCurve(hashData);
                auto hashSet = m_config->eccCrypto()->ecMultiply(point, *m_randomB);
                encryptedHashSet[i] = hashSet;
            }
        });
        while (readCount < inputSize)
        {
            if (inputSize < batchSize)
            {
                readCount = batchSize;
            }
            else if (readCount + batchSize < inputSize)
            {
                readCount += batchSize;
            }
            else
            {
                readCount = inputSize;
            }

            std::vector<bcos::bytes> encryptedHashSetSplit;
            splitVector(encryptedHashSet, readStart, readCount, encryptedHashSetSplit);

            ECDH_MULTI_LOG(INFO)
                << LOG_KV("Part3: Master compute the H(Z)*B encryptedHashSet success size: ",
                       encryptedHashSet.size())
                << LOG_KV("Part3: Master compute the H(Z)*B encryptedHashSetSplit success size: ",
                       encryptedHashSetSplit.size());

            for (auto& calcultor : m_calculatorParties)
            {
                ECDH_MULTI_LOG(INFO)
                    << LOG_KV(
                           " Send the EncryptSet H(Z)*B success to Calculator: ", calcultor.first)
                    << LOG_KV("EncryptSet Sample: ", encryptedHashSet[0].data());
                auto message = m_config->psiMsgFactory()->createPSIMessage(
                    uint32_t(EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_CALCULATOR));
                message->setData(encryptedHashSetSplit);
                message->setDataBatchCount(needSendTimes);
                message->setFrom(m_taskState->task()->selfParty()->id());
                m_config->generateAndSendPPCMessage(
                    calcultor.first, m_taskID, message,
                    [self = weak_from_this()](bcos::Error::Ptr&& _error) {
                        if (!_error)
                        {
                            return;
                        }
                        auto psi = self.lock();
                        if (!psi)
                        {
                            return;
                        }
                    },
                    readCount);
            }
            readStart += batchSize;
        }
    }
    catch (std::exception& e)
    {
        ECDH_MULTI_LOG(INFO)
            << LOG_DESC("Exception in onHandlerEncryptIntersectionSetFromCalculatorToMaster:")
            << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

void EcdhMultiPSIMaster::onHandlerSyncFinalResultToAllPeer(PSIMessageInterface::Ptr _msg)
{
    ECDH_MULTI_LOG(INFO)
        << LOG_KV("Final: Master Get SyncFinalResultToAllPeer From Calculator : ",
               _msg->takeData().size())
        << LOG_KV("Final: Master Get SyncFinalResultToAllPeer From Calculator count: ",
               m_final_counts[m_taskID])
        << LOG_KV("Final: Master isSyncedResult:", m_syncResult);

    if (m_syncResult)
    {
        auto needTimes = _msg->dataBatchCount();
        auto res = _msg->takeData();

        trimVector(res);
        bcos::WriteGuard l(x_final_count);
        m_final_vectors.insert(m_final_vectors.end(), res.begin(), res.end());
        m_final_counts[m_taskID]++;
        if (m_final_counts[m_taskID] < needTimes)
        {
            return;
        }

        m_taskState->storePSIResult(m_config->dataResourceLoader(), m_final_vectors);
        ECDH_MULTI_LOG(INFO) << LOG_KV(
            "Final: Master onHandlerSyncFinalResultToAllPeer Store Intersection_XY^b ∩ H(Z)^b^a "
            "Success"
            "Dataset size: ",
            m_final_vectors.size());
    }
    else
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Master:No Need To ReceiveResultFromCalculator");
    }

    m_taskState->setFinished(true);
    m_taskState->onTaskFinished();
}

bool EcdhMultiPSIMaster::loadCipherDataFinished()
{
    auto allPeerParties = m_taskState->task()->getAllPeerParties();
    auto finishedPeers = m_masterCipherDataCache->masterTaskPeersFinishedList();
    if (allPeerParties.size() == finishedPeers.size())
    {
        for (auto& _peer : allPeerParties)
        {
            if (!finishedPeers.contains(_peer.first))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

void EcdhMultiPSIMaster::onTaskError(std::string&& _error)
{
    auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
    auto err = std::make_shared<bcos::Error>(-12222, _error);
    result->setError(std::move(err));
    m_taskState->onTaskFinished(result, true);
}