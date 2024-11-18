#include "EcdhMultiPSIImpl.h"
#include "Common.h"
#include "ppc-framework/protocol/Constant.h"
#include <gperftools/malloc_extension.h>

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace bcos;
using namespace ppc::task;


EcdhMultiPSIImpl::EcdhMultiPSIImpl(const EcdhMultiPSIConfig::Ptr& _config, unsigned _idleTimeMs)
  : m_config(std::move(_config)),
    m_msgQueue(std::make_shared<EcdhMultiPSIMsgQueue>()),
    TaskGuarder(_config, TaskAlgorithmType::ECDH_PSI_MULTI, "ECDH-MULTI-PSI-Timer")
{}

void EcdhMultiPSIImpl::onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg)
{
    try
    {
        m_msgQueue->push(_msg);
        wakeupWorker();
    }
    catch (std::exception const& e)
    {
        ECDH_MULTI_LOG(WARNING) << LOG_DESC("onReceiveMessage exception") << printPPCMsg(_msg)
                                << LOG_KV("error", boost::diagnostic_information(e));
    }
}

void EcdhMultiPSIImpl::handlerPSIReceiveMessage(PSIMessageInterface::Ptr _msg)
{
    auto self = weak_from_this();
    m_config->threadPool()->enqueue([self, _msg]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        try
        {
            switch (_msg->packetType())
            {
            case (uint32_t)EcdhMultiPSIMessageType::GENERATE_RANDOM_TO_PARTNER:
            {
                // calculator -> partner (A)
                psi->onReceiveRandomA(_msg);
                break;
            }
            case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_CALCULATOR:
            {
                // calculator -> master H(X)*A
                psi->onReceiveCalCipher(_msg);
                break;
            }
            case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_MASTER_FROM_PARTNER:
            {
                // patner -> master H(Y)*A
                psi->onReceiveCipherFromPartner(_msg);
                break;
            }
            case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_INTERSECTION_SET_TO_CALCULATOR:
            {
                psi->onReceiveIntersecCipher(_msg);
                break;
            }
            case (uint32_t)EcdhMultiPSIMessageType::SEND_ENCRYPTED_SET_TO_CALCULATOR:
            {
                psi->onReceiveMasterCipher(_msg);
                break;
            }
            case (uint32_t)EcdhMultiPSIMessageType::
                RETURN_ENCRYPTED_INTERSECTION_SET_FROM_CALCULATOR_TO_MASTER:
            {
                break;
            }
            case (uint32_t)EcdhMultiPSIMessageType::SYNC_FINAL_RESULT_TO_ALL:
            {
                psi->onReceivePSIResult(_msg);
                break;
            }
            default:
            {
                ECDH_MULTI_LOG(WARNING)
                    << LOG_DESC("Unsupported packetType ") << (int)_msg->packetType();
                break;
            }
            }
        }
        catch (std::exception const& e)
        {
            ECDH_MULTI_LOG(WARNING)
                << LOG_DESC("handlePSIMsg exception") << LOG_KV("packetType", _msg->packetType())
                << printPSIMessage(_msg) << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}


void EcdhMultiPSIImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, ppc::task::TaskResponseCallback&& _onTaskFinished)
{
    auto taskID = _task->id();
    auto taskState =
        m_taskStateFactory->createTaskState(_task, std::move(_onTaskFinished), false, m_config);
    auto self = weak_from_this();
    taskState->registerNotifyPeerFinishHandler([self, _task]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        psi->noticePeerToFinish(_task);
    });
    taskState->registerFinalizeHandler([self, taskID]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        // erase the taskInfo from the gateway
        psi->m_config->front()->eraseTaskInfo(taskID);
        psi->removeCalculator(taskID);
        psi->removeMaster(taskID);
        psi->removePartner(taskID);
        psi->removePendingTask(taskID);
    });
    addPendingTask(taskState);
    // over the peer limit
    if (_task->getAllPeerParties().size() > c_max_peer_size)
    {
        auto error = std::make_shared<bcos::Error>(
            -1, "at most support " + std::to_string(c_max_peer_size) + " peers, over the limit!");
        ECDH_MULTI_LOG(WARNING) << LOG_DESC("asyncRunTask failed")
                                << LOG_KV("msg", error->errorMessage());
        onSelfError(_task->id(), error, true);
    }
    try
    {
        auto dataResource = _task->selfParty()->dataResource();
        auto reader = loadReader(_task->id(), dataResource, DataSchema::Bytes);
        auto sqlReader = (reader->type() == ppc::protocol::DataResourceType::MySQL);
        auto nextParam = sqlReader ? 0 : m_config->dataBatchSize();
        taskState->setReader(reader, nextParam);
        auto role = _task->selfParty()->partyIndex();
        auto receivers = _task->getReceiverLists();
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Start a asyncRunTask ") << LOG_KV("taskID", _task->id())
                             << LOG_KV("roleId", role);
        if (role == uint16_t(PartiesType::Calculator))
        {
            auto writer = loadWriter(_task->id(), dataResource, m_enableOutputExists);
            taskState->setWriter(writer);
            ECDH_MULTI_LOG(INFO) << LOG_DESC("Calculator do the Task")
                                 << LOG_KV("taskID", _task->id());
            auto calculator = std::make_shared<EcdhMultiPSICalculator>(m_config, taskState);
            calculator->asyncStartRunTask(_task);
            addCalculator(std::move(calculator));
        }
        else if (role == uint16_t(PartiesType::Partner))
        {
            ECDH_MULTI_LOG(INFO) << LOG_DESC("Partner do the Task")
                                 << LOG_KV("taskID", _task->id());
            if (_task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                 m_config->selfParty()) != receivers.end())
            {
                auto writer = loadWriter(_task->id(), dataResource, m_enableOutputExists);
                taskState->setWriter(writer);
            }
            auto partner = std::make_shared<EcdhMultiPSIPartner>(m_config, taskState);
            partner->asyncStartRunTask(_task);
            addPartner(std::move(partner));
        }
        else if (role == uint16_t(PartiesType::Master))
        {
            ECDH_MULTI_LOG(INFO) << LOG_DESC("Master do the Task") << LOG_KV("taskID", _task->id());
            if (_task->syncResultToPeer() && std::find(receivers.begin(), receivers.end(),
                                                 m_config->selfParty()) != receivers.end())
            {
                auto writer = loadWriter(_task->id(), dataResource, m_enableOutputExists);
                taskState->setWriter(writer);
            }
            auto master = std::make_shared<EcdhMultiPSIMaster>(m_config, taskState);
            master->asyncStartRunTask(_task);
            addMaster(std::move(master));
        }
        else
        {
            BOOST_THROW_EXCEPTION(ECDHMULTIException() << bcos::errinfo_comment(
                                      "The party index of the ecdh-multi-psi must be calculator(0) "
                                      "or partner(1) or master(2)!"));
        }

        // notify the taskInfo to the front
        m_config->front()->notifyTaskInfo(_task->id());
    }
    catch (bcos::Error const& e)
    {
        ECDH_MULTI_LOG(ERROR) << LOG_DESC("asyncRunTask exception") << printTaskInfo(_task)
                              << LOG_KV("code", e.errorCode()) << LOG_KV("msg", e.errorMessage());
        onSelfError(
            _task->id(), std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), true);
    }
    catch (std::exception& e)
    {
        auto error = BCOS_ERROR_PTR((int)TaskParamsError, boost::diagnostic_information(e));
        onSelfError(_task->id(), error, true);
    }
}

void EcdhMultiPSIImpl::start()
{
    startWorking();
    startPingTimer();
}

void EcdhMultiPSIImpl::stop()
{
    if (m_config->threadPool())
    {
        m_config->threadPool()->stop();
    }

    finishWorker();
    if (isWorking())
    {
        // stop the worker thread
        stopWorking();
        terminate();
    }
    stopPingTimer();
}

void EcdhMultiPSIImpl::checkFinishedTask()
{
    std::set<std::string> finishedTask;
    {
        bcos::WriteGuard l(x_pendingTasks);
        if (m_pendingTasks.empty())
        {
            return;
        }

        for (auto it = m_pendingTasks.begin(); it != m_pendingTasks.end();)
        {
            auto task = it->second;
            if (task->taskDone())
            {
                finishedTask.insert(it->first);
            }
            it++;
        }
    }
    for (auto& taskID : finishedTask)
    {
        removeCalculator(taskID);
        removeMaster(taskID);
        removePartner(taskID);
        removePendingTask(taskID);
    }
}

void EcdhMultiPSIImpl::onReceivedErrorNotification(const std::string& _taskID)
{
    ECDH_MULTI_LOG(INFO) << LOG_DESC("onReceivedErrorNotification") << LOG_KV("taskID", _taskID);
    // finish the task while the peer is failed
    auto taskState = findPendingTask(_taskID);
    if (taskState)
    {
        taskState->onPeerNotifyFinish();

        wakeupWorker();
    }
}

void EcdhMultiPSIImpl::onSelfError(
    const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer)
{
    auto taskState = findPendingTask(_taskID);
    if (!taskState)
    {
        return;
    }

    ECDH_MULTI_LOG(ERROR) << LOG_DESC("onSelfError") << LOG_KV("task", _taskID)
                          << LOG_KV("exception", _error->errorMessage())
                          << LOG_KV("noticePeer", _noticePeer);

    auto result = std::make_shared<TaskResult>(taskState->task()->id());
    result->setError(std::move(_error));
    taskState->onTaskFinished(result, _noticePeer);

    wakeupWorker();
}

void EcdhMultiPSIImpl::executeWorker()
{
    checkFinishedTask();
    auto _msg = m_msgQueue->tryPop(c_popWaitMs);
    if (_msg.first)
    {
        auto pop_msg = _msg.second;
        if (pop_msg->messageType() == uint8_t(CommonMessageType::ErrorNotification))
        {
            onReceivedErrorNotification(pop_msg->taskID());
            return;
        }
        else if (pop_msg->messageType() == uint8_t(CommonMessageType::PingPeer))
        {
            return;
        }

        // decode the psi message
        auto payLoad = pop_msg->data();
        auto psiMsg = m_config->psiMsgFactory()->decodePSIMessage(
            bcos::bytesConstRef(payLoad->data(), payLoad->size()));
        psiMsg->setFrom(pop_msg->sender());
        psiMsg->setTaskID(pop_msg->taskID());
        psiMsg->setSeq(pop_msg->seq());
        psiMsg->setUUID(pop_msg->uuid());
        ECDH_MULTI_LOG(TRACE) << LOG_DESC("onReceiveMessage") << printPSIMessage(psiMsg)
                              << LOG_KV("uuid", psiMsg->uuid());
        // release the large payload immediately
        if (payLoad && payLoad->size() >= ppc::protocol::LARGE_MSG_THRESHOLD)
        {
            ECDH_MULTI_LOG(INFO) << LOG_DESC("Release large message payload")
                                 << LOG_KV("size", payLoad->size());
            pop_msg->releasePayload();
            MallocExtension::instance()->ReleaseFreeMemory();
        }
        handlerPSIReceiveMessage(psiMsg);
        return;
    }
    waitSignal();
}

void EcdhMultiPSIImpl::onReceiveRandomA(PSIMessageInterface::Ptr _msg)
{
    auto partner = findPartner(_msg->taskID());
    if (partner)
    {
        if (_msg->takeData().size() == 1)
        {
            auto msgData = _msg->getData(0);
            partner->onReceiveRandomA(
                std::make_shared<bcos::bytes>(bcos::bytes(msgData.begin(), msgData.end())));
        }
    }
}

void EcdhMultiPSIImpl::onReceiveCalCipher(PSIMessageInterface::Ptr _msg)
{
    auto master = findMaster(_msg->taskID());
    if (master)
    {
        master->onReceiveCalCipher(_msg);
    }
}

void EcdhMultiPSIImpl::onReceiveCipherFromPartner(PSIMessageInterface::Ptr _msg)
{
    auto master = findMaster(_msg->taskID());
    if (master)
    {
        master->onReceiveCipherFromPartner(_msg);
    }
}

void EcdhMultiPSIImpl::onReceiveIntersecCipher(PSIMessageInterface::Ptr _msg)
{
    auto calculator = findCalculator(_msg->taskID());
    if (calculator)
    {
        calculator->onReceiveIntersecCipher(_msg);
    }
}

void EcdhMultiPSIImpl::onReceiveMasterCipher(PSIMessageInterface::Ptr _msg)
{
    auto calculator = findCalculator(_msg->taskID());
    if (calculator)
    {
        calculator->onReceiveMasterCipher(_msg);
    }
}


void EcdhMultiPSIImpl::onReceivePSIResult(PSIMessageInterface::Ptr _msg)
{
    ECDH_MULTI_LOG(INFO) << LOG_DESC("onReceivePSIResult") << printPSIMessage(_msg);
    auto startT = utcSteadyTime();
    auto master = findMaster(_msg->taskID());
    if (master)
    {
        master->onReceivePSIResult(_msg);
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Master onReceivePSIResult finished")
                             << printPSIMessage(_msg)
                             << LOG_KV("timecost", (utcSteadyTime() - startT));
        return;
    }

    auto partner = findPartner(_msg->taskID());
    if (partner)
    {
        partner->onReceivePSIResult(_msg);
        ECDH_MULTI_LOG(INFO) << LOG_DESC("Partner onReceivePSIResult finished")
                             << printPSIMessage(_msg)
                             << LOG_KV("timecost", (utcSteadyTime() - startT));
        return;
    }
}
