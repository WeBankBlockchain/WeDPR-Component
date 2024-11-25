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
    m_masterCache = std::make_shared<MasterCache>(m_taskState, m_config);
    m_syncResult = (task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                    m_config->selfParty()) != receivers.end());
}

void EcdhMultiPSIMaster::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    ECDH_MASTER_LOG(INFO) << LOG_DESC("Master asyncStartRunTask") << printTaskInfo(_task);
    initTask(_task);
    auto B = m_config->eccCrypto()->generateRandomScalar();
    m_randomB = std::make_shared<bcos::bytes>(B);
    auto self = weak_from_this();
    m_config->threadPool()->enqueue([self]() {
        auto master = self.lock();
        if (!master)
        {
            return;
        }
        ECDH_MASTER_LOG(INFO) << LOG_DESC("Master blindData") << LOG_KV("task", master->m_taskID);
        master->blindData();
    });
}

void EcdhMultiPSIMaster::initTask(ppc::protocol::Task::ConstPtr _task)
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
void EcdhMultiPSIMaster::onReceiveCalCipher(PSIMessageInterface::Ptr _msg)
{
    try
    {
        ECDH_MASTER_LOG(INFO) << LOG_DESC("onReceiveCalCipher") << printPSIMessage(_msg);
        auto&& data = _msg->takeData();
        auto const& dataIndex = _msg->dataIndex();
        m_masterCache->addCalculatorCipher(
            _msg->from(), std::move(data), dataIndex, _msg->seq(), _msg->dataBatchCount());
        auto ret = m_masterCache->tryToIntersection();
        if (!ret)
        {
            return;
        }
        m_masterCache->encryptIntersection(*m_randomB, m_calculatorParties);
    }
    catch (std::exception& e)
    {
        ECDH_MASTER_LOG(WARNING) << LOG_DESC("Exception in onReceiveCalCipher:")
                                 << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

// Part1-P,2-P: Partner -> Master [H(X)*A ∩ H(Y)*A]*B
void EcdhMultiPSIMaster::onReceiveCipherFromPartner(PSIMessageInterface::Ptr _msg)
{
    try
    {
        ECDH_MASTER_LOG(INFO) << LOG_DESC("onReceiveCipherFromPartner") << printPSIMessage(_msg);
        m_masterCache->addPartnerCipher(
            _msg->from(), _msg->takeData(), _msg->seq(), _msg->dataBatchCount());
        auto ret = m_masterCache->tryToIntersection();
        if (!ret)
        {
            return;
        }
        m_masterCache->encryptIntersection(*m_randomB, m_calculatorParties);
    }
    catch (std::exception& e)
    {
        ECDH_MASTER_LOG(WARNING) << LOG_DESC("Exception in onReceiveCipherFromPartner:")
                                 << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

// Part3: Master -> Calculator H(Z)*B
void EcdhMultiPSIMaster::blindData()
{
    try
    {
        ECDH_MULTI_LOG(INFO) << LOG_DESC("blindData") << LOG_KV("taskID", m_taskID);
        auto startT = utcSteadyTime();
        auto reader = m_taskState->reader();
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
                    ECDH_CAL_LOG(INFO) << LOG_DESC("blindData return for all data loaded")
                                       << LOG_KV("task", m_taskState->task()->id());
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
            auto startT = utcSteadyTime();
            ECDH_MASTER_LOG(INFO) << LOG_DESC("blindData: encrypt data") << LOG_KV("seq", seq)
                                  << LOG_KV("task", m_taskState->task()->id())
                                  << LOG_KV("dataSize", dataBatch->size());
            std::vector<bcos::bytes> cipher(dataBatch->size());
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0U, dataBatch->size()), [&](auto const& range) {
                    for (auto i = range.begin(); i < range.end(); i++)
                    {
                        auto const& data = dataBatch->get<bcos::bytes>(i);
                        auto hashData =
                            m_config->hash()->hash(bcos::bytesConstRef(data.data(), data.size()));
                        auto point = m_config->eccCrypto()->hashToCurve(hashData);
                        cipher[i] = m_config->eccCrypto()->ecMultiply(point, *m_randomB);
                    }
                });
            // can release databatch after encrypted
            dataBatch->release();
            // free after release
            MallocExtension::instance()->ReleaseFreeMemory();
            ECDH_MASTER_LOG(INFO) << LOG_DESC("blindData: encrypt data success")
                                  << LOG_KV("dataSize", cipher.size()) << LOG_KV("task", m_taskID)
                                  << LOG_KV("seq", seq)
                                  << LOG_KV("timecost", (utcSteadyTime() - startT));

            ECDH_MASTER_LOG(INFO) << LOG_DESC("blindData: send encrypted data to all calculator")
                                  << LOG_KV("seq", seq) << LOG_KV("task", m_taskID)
                                  << LOG_KV("calculators", m_calculatorParties.size());
            auto message = m_config->psiMsgFactory()->createPSIMessage(
                uint32_t(EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_CALCULATOR));
            message->setData(cipher);
            if (reader->readFinished())
            {
                message->setDataBatchCount(m_taskState->sendedDataBatchSize());
            }
            else
            {
                // 0 means not finished
                message->setDataBatchCount(0);
            }
            message->setFrom(m_taskState->task()->selfParty()->id());
            auto self = weak_from_this();
            for (auto const& calcultor : m_calculatorParties)
            {
                m_config->generateAndSendPPCMessage(
                    calcultor.first, m_taskID, message,
                    [self, seq, calcultor](bcos::Error::Ptr&& _error) {
                        if (!_error || _error->errorCode() == 0)
                        {
                            return;
                        }
                        auto psi = self.lock();
                        if (!psi)
                        {
                            return;
                        }
                        ECDH_MASTER_LOG(WARNING)
                            << LOG_DESC("send blindedData to calculator failed")
                            << LOG_KV("calculator", calcultor.first) << LOG_KV("seq", seq)
                            << LOG_KV("task", psi->m_taskState->task()->id())
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
                        psi->m_taskState->onTaskException(_error->errorMessage());
                    },
                    seq);
            }
        } while (!m_taskState->sqlReader());
    }
    catch (std::exception& e)
    {
        ECDH_MASTER_LOG(WARNING) << LOG_DESC("blindData exception")
                                 << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

void EcdhMultiPSIMaster::onReceivePSIResult(PSIMessageInterface::Ptr _msg)
{
    ECDH_MASTER_LOG(INFO) << LOG_DESC("onReceivePSIResult") << printPSIMessage(_msg);
    if (m_syncResult)
    {
        m_taskState->storePSIResult(m_config->dataResourceLoader(), _msg->takeData());
        ECDH_MASTER_LOG(INFO) << LOG_DESC("onReceivePSIResult: store psi result success")
                              << printPSIMessage(_msg);
    }
    else
    {
        ECDH_MASTER_LOG(INFO) << LOG_DESC("Master:No Need To store the psi result")
                              << printPSIMessage(_msg);
    }

    m_taskState->setFinished(true);
    m_taskState->onTaskFinished();
}


void EcdhMultiPSIMaster::onTaskError(std::string&& _error)
{
    auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
    auto err = std::make_shared<bcos::Error>(-12222, _error);
    result->setError(std::move(err));
    m_taskState->onTaskFinished(result, true);
}