#include "EcdhMultiPSICalculator.h"
#include "ppc-psi/src/ecdh-multi-psi/Common.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <tbb/parallel_for.h>

using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace bcos;

EcdhMultiPSICalculator::EcdhMultiPSICalculator(
    EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState)
  : m_config(std::move(_config)), m_taskState(std::move(_taskState))
{
    auto task = m_taskState->task();
    auto receivers = task->getReceiverLists();
    m_taskID = task->id();
    m_calculatorCipherDataCache = std::make_shared<CalculatorCipherDataCache>();
    m_syncResult = (task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                    m_config->selfParty()) != receivers.end());
}

void EcdhMultiPSICalculator::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    InitAsyncTask(_task);
    auto randA = generateRandomA(_task->id());
    m_randomA = randA;
    m_config->threadPool()->enqueue([self = weak_from_this(), _task, randA]() {
        auto calculator = self.lock();
        if (!calculator)
        {
            return;
        }
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Calculator asyncStartRunTask as calculator");
        calculator->computeAndEncryptSet(_task->id(), randA);
    });
}

// PART1: Calculator -> Partners (A)
bcos::bytes EcdhMultiPSICalculator::generateRandomA(std::string _taskID)
{
    ECDH_MULTI_LOG(INFO) << LOG_KV("PART1: Calculator Start New Task :", _taskID);
    auto A = m_config->eccCrypto()->generateRandomScalar();
    // send to all partners
    auto message = m_config->psiMsgFactory()->createPSIMessage(
        uint32_t(EcdhMultiPSIMessageType::GENERATE_RANDOM_TO_PARTNER));
    message->setData(std::vector<bcos::bytes>{A});
    message->setDataBatchCount(A.size());
    message->setFrom(m_taskState->task()->selfParty()->id());
    for (auto& partner : m_partnerParties)
    {
        ECDH_MULTI_LOG(INFO) << LOG_KV("PART1: Calculator generateRandomA to ", partner.first)
                             << LOG_KV(" Random A ", *toHexString(A));
        m_config->generateAndSendPPCMessage(
            partner.first, _taskID, message,
            [self = weak_from_this(), partner](bcos::Error::Ptr&& _error) {
                if (!_error)
                {
                    ECDH_MULTI_LOG(INFO)
                        << LOG_KV("PART1: Calculator generateRandomA success to ", partner.first);
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
    return A;
}

// PART2: Calculator -> Master H(X)*A
void EcdhMultiPSICalculator::computeAndEncryptSet(std::string _taskID, bcos::bytes _randA)
{
    ECDH_MULTI_LOG(INFO) << LOG_KV(
        "PART2:Calculator send to Master Use RandomA: ", *toHexString(_randA));
    auto startT = utcSteadyTime();
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
    ECDH_MULTI_LOG(INFO) << LOG_KV("PART2:Calculator Send Data Batch Times: ", needSendTimes);
    try
    {
        auto hash = m_config->hash();
        uint32_t readStart = 0, readCount = 0;
        tbb::concurrent_map<uint32_t, bcos::bytes> encryptedHashMap;
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto data = m_originInputs->getBytes(i);
                auto hashData = hash->hash(bcos::bytesConstRef(data.data(), data.size()));
                auto point = m_config->eccCrypto()->hashToCurve(hashData);
                auto hashSet = m_config->eccCrypto()->ecMultiply(point, _randA);
                // encryptedHashMap.insert(std::make_pair(i, hashSet));
                encryptedHashMap.emplace(std::make_pair(i, hashSet));
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
            std::map<uint32_t, bcos::bytes> cHashMap;
            ConcurrentSTLToCommon(encryptedHashMap, readStart, readCount, cHashMap);
            ECDH_MULTI_LOG(INFO) << LOG_KV(
                "PART2:Calculator compute the H(X)*A encryptedHashMap success size: ",
                cHashMap.size());
            for (auto& master : m_masterParties)
            {
                auto message = m_config->psiMsgFactory()->createPSIMessage(uint32_t(
                    EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_CALCULATOR));
                message->setDataMap(std::move(cHashMap));
                message->setFrom(m_taskState->task()->selfParty()->id());
                message->setDataBatchCount(needSendTimes);
                m_config->generateAndSendPPCMessage(
                    master.first, _taskID, message,
                    [self = weak_from_this(), master](bcos::Error::Ptr&& _error) {
                        if (!_error)
                        {
                            ECDH_MULTI_LOG(INFO) << LOG_KV(
                                "PART2:Send the EncryptSet success to Master: ", master.first);
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
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Exception in computeAndEncryptSet:")
                             << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}


void EcdhMultiPSICalculator::InitAsyncTask(ppc::protocol::Task::ConstPtr _task)
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

// Part3: Calculator store Intersection_XY^b <- Master (response)
void EcdhMultiPSICalculator::onHandlerIntersectEncryptSetToCalculator(PSIMessageInterface::Ptr _msg)
{
    auto encryptedMap = _msg->takeDataMap();
    ECDH_MULTI_LOG(INFO) << LOG_KV(
        "Part3: Calculator Receive Intersection_XY^b onHandlerIntersectEncryptSetToCalculator "
        "Received Dataset size: ",
        encryptedMap.size());
    try
    {
        auto seq = _msg->seq();
        auto needSendTimes = _msg->dataBatchCount();
        bool finished = m_calculatorCipherDataCache->setCalculatorIntersectionCipherDataMap(
            std::move(encryptedMap), seq, needSendTimes);
        if (finished == false)
        {
            return;
        }
        for (auto& master : m_masterParties)
        {
            ECDH_MULTI_LOG(INFO) << LOG_KV(
                "Part3: onHandlerIntersectEncryptSetToCalculator Send the Response to Master: ",
                master.first);
            auto message =
                m_config->psiMsgFactory()->createPSIMessage(uint32_t(EcdhMultiPSIMessageType::
                        RETURN_ENCRYPTED_INTERSECTION_SET_FROM_CALCULATOR_TO_MASTER));
            message->setFrom(m_taskState->task()->selfParty()->id());
            m_config->generateAndSendPPCMessage(
                master.first, m_taskID, message,
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
    }
    catch (std::exception& e)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Exception in onHandlerIntersectEncryptSetToCalculator:")
                             << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

// Part4 : Intersection_XY^b ∩ H(Z)^b^a
void EcdhMultiPSICalculator::onHandlerEncryptSetToCalculator(PSIMessageInterface::Ptr _msg)
{
    auto encryptedSet = _msg->takeData();
    auto inputSize = encryptedSet.size();
    ECDH_MULTI_LOG(INFO) << LOG_KV(
        " Part4: Master onHandlerEncryptSetToCalculator Received H(Z)*B Dataset size: ", inputSize);
    std::vector<bcos::bytes> encryptedHashSet;
    encryptedHashSet.reserve(inputSize);
    encryptedHashSet.resize(inputSize);
    try
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto data = encryptedSet.at(i);
                if (data.data())
                {
                    auto hashSet = m_config->eccCrypto()->ecMultiply(data, m_randomA);
                    encryptedHashSet[i] = hashSet;
                }
            }
        });

        auto needTimes = _msg->dataBatchCount();
        auto seq = _msg->seq();
        bool finished = m_calculatorCipherDataCache->setCalculatorCipherData(
            std::move(encryptedHashSet), seq, needTimes);
        if (finished == false)
        {
            return;
        }
        m_calculatorCipherDataCache->tryToGetCipherDataIntersection();
        auto cipherDataResult =
            m_calculatorCipherDataCache->calculatorIntersectionCipherDataFinalMap();
        ECDH_MULTI_LOG(INFO) << LOG_KV(
            "Part4: Master onHandlerEncryptSetToCalculator "
            "Intersection_XY^b ∩ H(Z)^b^a Dataset "
            "size: ",
            cipherDataResult.size());

        if (!m_originInputs)
        {
            m_originInputs = m_taskState->loadAllData();
            if (!m_originInputs || m_originInputs->size() == 0)
            {
                BOOST_THROW_EXCEPTION(
                    ECDHMULTIException() << bcos::errinfo_comment("data is empty"));
            }
        }

        m_finalResults.clear();
        if (cipherDataResult.size() > 0)
        {
            for (auto& _res : cipherDataResult)
            {
                m_finalResults.push_back(m_originInputs->getBytes(_res.first));
            }
        }

        ECDH_MULTI_LOG(INFO) << LOG_KV(
            "Part4: onHandlerEncryptSetToCalculator Store "
            "Intersection_XY^b ∩ H(Z)^b^a Success "
            "Dataset size: ",
            cipherDataResult.size());
        // sync Result or status to All peers
        syncResultToAllPeers();
        m_taskState->storePSIResult(m_config->dataResourceLoader(), m_finalResults);

        // return the rpc
        m_taskState->setFinished(true);
        m_taskState->onTaskFinished();
    }
    catch (std::exception& e)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Exception in onHandlerEncryptSetToCalculator:")
                             << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}


void EcdhMultiPSICalculator::syncResultToAllPeers()
{
    auto all_peers = m_taskState->task()->getAllPeerParties();
    ECDH_MULTI_LOG(INFO) << LOG_KV("Calculator: Calculator isSyncedResult:", m_syncResult);
    if (!m_syncResult)
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Calculator:No Need To SyncResultToAllPeers");
        for (auto& _peer : all_peers)
        {
            auto message = m_config->psiMsgFactory()->createPSIMessage(
                uint32_t(EcdhMultiPSIMessageType::SYNC_FINAL_RESULT_TO_ALL));
            message->setFrom(m_taskState->task()->selfParty()->id());
            message->setVersion(-1);
            m_config->generateAndSendPPCMessage(
                _peer.first, m_taskID, message,
                [self = weak_from_this(), _peer](bcos::Error::Ptr&& _error) {
                    if (!_error)
                    {
                        ECDH_MULTI_LOG(INFO)
                            << LOG_KV("Calculator:Calculator sync no Need Sync To Peer Success:",
                                   _peer.first);
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
    auto batchSize = m_config->dataBatchSize();
    auto inputSize = m_finalResults.size();
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

    ECDH_MULTI_LOG(INFO) << LOG_KV(
        "Final:Calculator Send Final Intersection Batch Times: ", needSendTimes);
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

        std::vector<bcos::bytes> chunk_final_result;
        splitVector(m_finalResults, readStart, readCount, chunk_final_result);

        ECDH_MULTI_LOG(INFO) << LOG_KV(
            "Final:Calculator Send Final Intersection Chunk Size: ", chunk_final_result.size());
        for (auto& _peer : all_peers)
        {
            auto message = m_config->psiMsgFactory()->createPSIMessage(
                uint32_t(EcdhMultiPSIMessageType::SYNC_FINAL_RESULT_TO_ALL));
            message->setData(chunk_final_result);
            message->setFrom(m_taskState->task()->selfParty()->id());
            message->setVersion(0);
            message->setDataBatchCount(needSendTimes);
            m_config->generateAndSendPPCMessage(
                _peer.first, m_taskID, message,
                [self = weak_from_this(), _peer](bcos::Error::Ptr&& _error) {
                    if (!_error)
                    {
                        ECDH_MULTI_LOG(INFO)
                            << LOG_KV("Final:Calculator sync result To Peer Success:", _peer.first);
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

void EcdhMultiPSICalculator::onTaskError(std::string&& _error)
{
    auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
    auto err = std::make_shared<bcos::Error>(-12222, _error);
    result->setError(std::move(err));
    m_taskState->onTaskFinished(result, true);
}
