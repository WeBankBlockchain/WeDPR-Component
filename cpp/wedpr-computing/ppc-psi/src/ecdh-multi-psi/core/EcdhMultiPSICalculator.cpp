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
    m_syncResult = (task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                    m_config->selfParty()) != receivers.end());
    m_calculatorCache = std::make_shared<CalculatorCache>(m_taskState, m_syncResult, m_config);
}

void EcdhMultiPSICalculator::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    initTask(_task);
    auto randA = generateRandomA(_task->id());
    m_randomA = randA;
    m_config->threadPool()->enqueue([self = weak_from_this(), _task, randA]() {
        auto calculator = self.lock();
        if (!calculator)
        {
            return;
        }
        ECDH_CAL_LOG(INFO) << LOG_DESC("Calculator asyncStartRunTask as calculator");
        calculator->blindData(_task->id(), randA);
    });
}

// PART1: Calculator -> Partners (randomValue)
bcos::bytes EcdhMultiPSICalculator::generateRandomA(std::string _taskID)
{
    ECDH_CAL_LOG(INFO) << LOG_KV("PART1: Calculator Start New Task :", _taskID);
    auto randomValue = m_config->eccCrypto()->generateRandomScalar();
    // send to all partners
    auto message = m_config->psiMsgFactory()->createPSIMessage(
        uint32_t(EcdhMultiPSIMessageType::GENERATE_RANDOM_TO_PARTNER));
    message->setData(std::vector<bcos::bytes>{randomValue});
    message->setDataBatchCount(randomValue.size());
    message->setFrom(m_taskState->task()->selfParty()->id());
    for (auto& partner : m_partnerParties)
    {
        ECDH_CAL_LOG(INFO) << LOG_KV("PART1: Calculator generateRandomA to ", partner.first)
                           << LOG_KV(" Random: ", *toHexString(randomValue));
        m_config->generateAndSendPPCMessage(
            partner.first, _taskID, message,
            [self = weak_from_this(), partner](bcos::Error::Ptr&& _error) {
                if (!_error)
                {
                    ECDH_CAL_LOG(INFO)
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
    return randomValue;
}

// PART2: Calculator -> Master H(X)*A
void EcdhMultiPSICalculator::blindData(std::string _taskID, bcos::bytes _randA)
{
    try
    {
        ECDH_CAL_LOG(INFO) << LOG_DESC("blindData") << LOG_KV("task", _taskID);
        auto reader = m_taskState->reader();
        uint64_t dataOffset = 0;
        do
        {
            if (m_taskState->loadFinished())
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
                                       << LOG_KV("task", _taskID);
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
            m_calculatorCache->appendPlainData(dataBatch);
            // encrypt
            auto startT = utcSteadyTime();
            std::vector<bcos::bytes> encryptedData(dataBatch->size());
            tbb::parallel_for(
                tbb::blocked_range<size_t>(0U, dataBatch->size()), [&](auto const& range) {
                    for (auto i = range.begin(); i < range.end(); i++)
                    {
                        auto const& data = dataBatch->get<bcos::bytes>(i);
                        auto hashData =
                            m_config->hash()->hash(bcos::bytesConstRef(data.data(), data.size()));
                        auto point = m_config->eccCrypto()->hashToCurve(hashData);
                        encryptedData[i] = m_config->eccCrypto()->ecMultiply(point, _randA);
                    }
                });
            ECDH_CAL_LOG(INFO) << LOG_DESC("blindData encrypt success")
                               << LOG_KV("dataSize", encryptedData.size())
                               << LOG_KV("task", m_taskState->task()->id()) << LOG_KV("seq", seq)
                               << LOG_KV("timecost", (utcSteadyTime() - startT));
            ECDH_CAL_LOG(INFO) << LOG_DESC("blindData: send cipher to the master")
                               << LOG_KV("masterSize", m_masterParties.size())
                               << LOG_KV("dataSize", encryptedData.size())
                               << LOG_KV("task", m_taskState->task()->id());
            auto message = m_config->psiMsgFactory()->createPSIMessage(
                uint32_t(EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_CALCULATOR));
            message->constructDataMap(encryptedData, dataOffset);
            // release the encryptedData
            std::vector<bcos::bytes>().swap(encryptedData);
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
            // send cipher
            for (auto const& master : m_masterParties)
            {
                m_config->generateAndSendPPCMessage(
                    master.first, _taskID, message,
                    [self = weak_from_this(), master](bcos::Error::Ptr&& _error) {
                        if (!_error)
                        {
                            ECDH_CAL_LOG(INFO)
                                << LOG_KV("blindData success to Master: ", master.first);
                            return;
                        }
                        auto psi = self.lock();
                        if (!psi)
                        {
                            return;
                        }
                    },
                    seq);
                dataOffset += dataBatch->size();
            }
        } while (!m_taskState->sqlReader());
    }
    catch (std::exception& e)
    {
        ECDH_CAL_LOG(WARNING) << LOG_DESC("Exception in blindData:")
                              << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}


void EcdhMultiPSICalculator::initTask(ppc::protocol::Task::ConstPtr _task)
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
void EcdhMultiPSICalculator::onReceiveIntersecCipher(PSIMessageInterface::Ptr _msg)
{
    auto cipherData = _msg->takeDataMap();
    ECDH_CAL_LOG(INFO) << LOG_DESC("onReceiveIntersecCipher") << printPSIMessage(_msg)
                       << LOG_KV("dataSize", cipherData.size());
    try
    {
        m_calculatorCache->setIntersectionCipher(std::move(cipherData));
        auto message = m_config->psiMsgFactory()->createPSIMessage(uint32_t(
            EcdhMultiPSIMessageType::RETURN_ENCRYPTED_INTERSECTION_SET_FROM_CALCULATOR_TO_MASTER));
        message->setFrom(m_taskState->task()->selfParty()->id());
        // try to finalize
        auto ret = m_calculatorCache->tryToFinalize();
        for (auto const& master : m_masterParties)
        {
            ECDH_CAL_LOG(INFO) << LOG_DESC("onReceiveIntersecCipher: send response to the master")
                               << LOG_KV("master", master.first) << printPSIMessage(_msg);
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
        if (!ret)
        {
            return;
        }
        // return the rpc
        m_taskState->setFinished(true);
        m_taskState->onTaskFinished();
    }
    catch (std::exception& e)
    {
        ECDH_CAL_LOG(WARNING) << LOG_DESC("Exception in onReceiveIntersecCipher:")
                              << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}

// Part4 : Intersection_XY^b ∩ H(Z)^b^a
void EcdhMultiPSICalculator::onReceiveMasterCipher(PSIMessageInterface::Ptr _msg)
{
    try
    {
        auto cipher = _msg->takeData();
        ECDH_CAL_LOG(INFO) << LOG_DESC("onReceiveMasterCipher") << printPSIMessage(_msg)
                           << LOG_KV("cipher", cipher.size());
        std::vector<bcos::bytes> encryptedCipher(cipher.size());
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, cipher.size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                encryptedCipher[i] = m_config->eccCrypto()->ecMultiply(cipher.at(i), m_randomA);
            }
        });
        // release the cipher
        std::vector<bcos::bytes>().swap(cipher);

        auto seq = _msg->seq();
        bool finished = m_calculatorCache->appendMasterCipher(
            std::move(encryptedCipher), seq, _msg->dataBatchCount());
        if (finished == false)
        {
            return;
        }
        auto ret = m_calculatorCache->tryToFinalize();
        if (!ret)
        {
            return;
        }
        // return the rpc
        m_taskState->setFinished(true);
        m_taskState->onTaskFinished();
    }
    catch (std::exception& e)
    {
        ECDH_CAL_LOG(WARNING) << LOG_DESC("Exception in onReceiveMasterCipher:")
                              << boost::diagnostic_information(e);
        onTaskError(boost::diagnostic_information(e));
    }
}


void EcdhMultiPSICalculator::onTaskError(std::string&& _error)
{
    auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
    auto err = std::make_shared<bcos::Error>(-12222, _error);
    result->setError(std::move(err));
    m_taskState->onTaskFinished(result, true);
}
