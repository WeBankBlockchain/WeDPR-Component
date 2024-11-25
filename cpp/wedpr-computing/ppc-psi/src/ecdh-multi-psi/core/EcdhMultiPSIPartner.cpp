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
    m_syncResult = (task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                    m_config->selfParty()) != receivers.end());
}

void EcdhMultiPSIPartner::initTask(ppc::protocol::Task::ConstPtr _task)
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
void EcdhMultiPSIPartner::onReceiveRandomA(bcos::bytesPointer _randA)
{
    try
    {
        ECDH_PARTNER_LOG(INFO) << LOG_DESC("onReceiveRandomA and blindData")
                               << printTaskInfo(m_taskState->task());
        auto reader = m_taskState->reader();
        uint64_t dataOffset = 0;
        do
        {
            if (m_taskState->loadFinished() || m_taskState->taskDone())
            {
                break;
            }
            DataBatch::Ptr dataBatch = nullptr;
            uint32_t seq = 0;
            {
                bcos::Guard l(m_mutex);
                // Note: next is not thread-safe
                dataBatch =
                    m_taskState->reader()->next(m_taskState->readerParam(), DataSchema::Bytes);
                if (!dataBatch)
                {
                    ECDH_PARTNER_LOG(INFO)
                        << LOG_DESC("blindData: encode partner cipher return for all data loaded")
                        << LOG_KV("task", m_taskID);
                    m_taskState->setFinished(true);
                    break;
                }
                // allocate seq
                seq = m_taskState->allocateSeq();
                if (m_taskState->sqlReader())
                {
                    m_taskState->setFinished(true);
                }
            }
            ECDH_PARTNER_LOG(INFO) << LOG_DESC("blindData: encode parterner cipher")
                                   << LOG_KV("size", dataBatch->size()) << LOG_KV("seq", seq)
                                   << LOG_KV("task", m_taskState->task()->id());
            auto startT = utcSteadyTime();
            std::vector<bcos::bytes> cipherData(dataBatch->size());
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0U, dataBatch->size()), [&](auto const& range) {
                    for (auto i = range.begin(); i < range.end(); i++)
                    {
                        auto const& data = dataBatch->get<bcos::bytes>(i);
                        auto hashData =
                            m_config->hash()->hash(bcos::bytesConstRef(data.data(), data.size()));
                        auto point = m_config->eccCrypto()->hashToCurve(hashData);
                        cipherData[i] = m_config->eccCrypto()->ecMultiply(point, *_randA);
                    }
                });
            ECDH_PARTNER_LOG(INFO)
                << LOG_DESC("blindData: encode parterner cipher success") << LOG_KV("seq", seq)
                << LOG_KV("task", m_taskState->task()->id()) << LOG_KV("size", dataBatch->size())
                << LOG_KV("timecost", (utcSteadyTime() - startT));

            ECDH_PARTNER_LOG(INFO)
                << LOG_DESC("blindData: send cipher data to master") << LOG_KV("seq", seq)
                << LOG_KV("size", dataBatch->size()) << LOG_KV("task", m_taskState->task()->id());
            auto message = m_config->psiMsgFactory()->createPSIMessage(
                uint32_t(EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_PARTNER));
            message->setData(std::move(cipherData));
            message->setFrom(m_taskState->task()->selfParty()->id());
            if (reader->readFinished())
            {
                message->setDataBatchCount(m_taskState->sendedDataBatchSize());
            }
            else
            {
                // 0 means not finished
                message->setDataBatchCount(0);
            }
            // generate and send encryptedHashSet
            auto self = weak_from_this();
            for (auto const& master : m_masterParties)
            {
                m_config->generateAndSendPPCMessage(
                    master.first, m_taskState->task()->id(), message,
                    [self, master, seq](bcos::Error::Ptr&& _error) {
                        if (!_error || _error->errorCode() == 0)
                        {
                            return;
                        }
                        auto psi = self.lock();
                        if (!psi)
                        {
                            return;
                        }
                        ECDH_PARTNER_LOG(WARNING)
                            << LOG_DESC("send blinded data to master failed") << LOG_KV("seq", seq)
                            << LOG_KV("master", master.first)
                            << LOG_KV("task", psi->m_taskState->task()->id())
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        psi->m_taskState->onTaskException(_error->errorMessage());
                    },
                    seq);
            }
            ECDH_PARTNER_LOG(INFO) << LOG_DESC("blindData: send cipher data to master success")
                                   << LOG_KV("size", dataBatch->size())
                                   << printTaskInfo(m_taskState->task()) << LOG_KV("seq", seq);
            dataBatch->release();
            // free after release
            MallocExtension::instance()->ReleaseFreeMemory();
        } while (!m_taskState->sqlReader());
    }
    catch (std::exception& e)
    {
        ECDH_PARTNER_LOG(WARNING) << LOG_DESC("onReceiveRandomA exception")
                                  << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

void EcdhMultiPSIPartner::onReceivePSIResult(PSIMessageInterface::Ptr _msg)
{
    ECDH_PARTNER_LOG(INFO) << LOG_DESC("onReceivePSIResult") << printPSIMessage(_msg);
    if (m_syncResult)
    {
        m_taskState->storePSIResult(m_config->dataResourceLoader(), _msg->takeData());
        ECDH_PARTNER_LOG(INFO) << LOG_DESC("onReceivePSIResult: store psi result success")
                               << printPSIMessage(_msg);
    }
    else
    {
        ECDH_PARTNER_LOG(INFO) << LOG_DESC("Master:No Need To store the psi result")
                               << printPSIMessage(_msg);
    }
    m_taskState->setFinished(true);
    m_taskState->onTaskFinished();
}


void EcdhMultiPSIPartner::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    initTask(_task);
    ECDH_PARTNER_LOG(INFO) << LOG_DESC("Partner asyncStartRunTask as partner")
                           << printTaskInfo(_task);
}


void EcdhMultiPSIPartner::onTaskError(std::string&& _error)
{
    auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
    auto err = std::make_shared<bcos::Error>(-12222, _error);
    result->setError(std::move(err));
    m_taskState->onTaskFinished(result, true);
}
