#include "EcdhMultiPSIPartner.h"
#include "ppc-psi/src/ecdh-multi-psi/Common.h"
#include <tbb/parallel_for.h>

using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace bcos;

EcdhMultiPSIPartner::EcdhMultiPSIPartner(EcdhMultiPSIConfig::Ptr _config, TaskState::Ptr _taskState)
  : m_config(std::move(_config)), m_taskState(std::move(_taskState))
{
    auto task = m_taskState->task();
    auto receivers = task->getReceiverLists();
    m_taskID = task->id();
    m_final_counts[m_taskID] = 0;
    m_syncResult = (task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                    m_config->selfParty()) != receivers.end());
}

void EcdhMultiPSIPartner::InitAsyncTask(ppc::protocol::Task::ConstPtr _task)
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

// PART1: Partner -> Master H(Y)*A
void EcdhMultiPSIPartner::oncomputeAndEncryptSet(bcos::bytesPointer _randA)
{
    try
    {
        ECDH_MULTI_LOG(INFO) << LOG_KV("Part1:Partner Receive RandomA: ", _randA->data());
        auto originInputs = m_taskState->loadAllData();
        if (!originInputs || originInputs->size() == 0)
        {
            BOOST_THROW_EXCEPTION(ECDHMULTIException() << bcos::errinfo_comment("data is empty"));
        }

        auto inputSize = originInputs->size();
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
        ECDH_MULTI_LOG(INFO) << LOG_KV(
            "Part1: Partner load the resource success size: ", inputSize);
        auto hash = m_config->hash();
        std::vector<bcos::bytes> encryptedHashSet;
        encryptedHashSet.reserve(inputSize);
        encryptedHashSet.resize(inputSize);
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, inputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto data = originInputs->getBytes(i);
                auto hashData = hash->hash(bcos::bytesConstRef(data.data(), data.size()));
                auto point = m_config->eccCrypto()->hashToCurve(hashData);
                auto hashSet = m_config->eccCrypto()->ecMultiply(point, *_randA);
                encryptedHashSet[i] = hashSet;
            }
        });
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

            std::vector<bcos::bytes> encryptedHashSetSplit;
            splitVector(encryptedHashSet, readStart, readCount, encryptedHashSetSplit);

            ECDH_MULTI_LOG(INFO)
                << LOG_KV("Part1: Partner compute the EncryptSet success encryptedHashSet size: ",
                       encryptedHashSet.size())
                << LOG_KV(
                       "Part1: Partner compute the EncryptSet success encryptedHashSetSplit "
                       "readStart: ",
                       readStart)
                << LOG_KV(
                       "Part1: Partner compute the EncryptSet success encryptedHashSetSplit "
                       "readCount: ",
                       readCount)
                << LOG_KV(
                       "Part1: Partner compute the EncryptSet success encryptedHashSetSplit "
                       "size: ",
                       encryptedHashSetSplit.size());

            // generate and send encryptedHashSet
            for (auto& master : m_masterParties)
            {
                auto message = m_config->psiMsgFactory()->createPSIMessage(
                    uint32_t(EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_PARTNER));
                message->setData(encryptedHashSetSplit);
                message->setFrom(m_taskState->task()->selfParty()->id());
                message->setDataBatchCount(needSendTimes);
                m_config->generateAndSendPPCMessage(
                    master.first, m_taskState->task()->id(), message,
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
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Exception in oncomputeAndEncryptSet:")
                             << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

void EcdhMultiPSIPartner::onHandlerSyncFinalResultToAllPeer(PSIMessageInterface::Ptr _msg)
{
    if (m_syncResult)
    {
        ECDH_MULTI_LOG(INFO)
            << LOG_KV("Final: Partner Get SyncFinalResultToAllPeer From Calculator : ",
                   _msg->takeData().size())
            << LOG_KV("Final: Partner Get SyncFinalResultToAllPeer From Calculator count: ",
                   m_final_counts[m_taskID])
            << LOG_KV("Final: Parter isSyncedResult:", m_syncResult);
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
            "Final: Partner onHandlerSyncFinalResultToAllPeer Store Intersection_XY^b ∩ H(Z)^b^a "
            "Success"
            "Dataset size: ",
            m_final_vectors.size());
    }
    else
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Partner:No Need To ReceiveResultFromCalculator");
    }

    m_taskState->setFinished(true);
    m_taskState->onTaskFinished();
}


void EcdhMultiPSIPartner::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    InitAsyncTask(_task);
    ECDH_MULTI_LOG(INFO) << LOG_DESC("Partner asyncStartRunTask as partner")
                         << printTaskInfo(_task);
}


void EcdhMultiPSIPartner::onTaskError(std::string&& _error)
{
    auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
    auto err = std::make_shared<bcos::Error>(-12222, _error);
    result->setError(std::move(err));
    m_taskState->onTaskFinished(result, true);
}
