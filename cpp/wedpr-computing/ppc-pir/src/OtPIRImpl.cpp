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
 * @file AYSService.cpp
 * @author: asherli
 * @date 2023-03-20
 */

#include "OtPIRImpl.h"
#include "BaseOT.h"
#include "Common.h"
#include "OtPIR.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-front/Common.h"
#include "wedpr-protocol/tars/TarsSerialize.h"
#include <bcos-utilities/BoostLog.h>


using namespace ppc;
using namespace bcos;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::pir;
using namespace ppc::task;
using namespace ppc::psi;


OtPIRImpl::OtPIRImpl(const OtPIRConfig::Ptr& _config, unsigned _idleTimeMs)
  : Worker("OT-PIR", _idleTimeMs),
    TaskGuarder(_config, TaskAlgorithmType::OT_PIR_2PC, "OT-PIR-Timer"),
    m_config(_config),
    m_msgQueue(std::make_shared<OtPIRMsgQueue>()),
    m_ioService(std::make_shared<boost::asio::io_service>()),
    m_parallelism(m_config->parallelism())
{
    m_senderThreadPool =
        std::make_shared<bcos::ThreadPool>("OT-PIR-Sender", std::thread::hardware_concurrency());
    m_ot = std::make_shared<crypto::BaseOT>(m_config->eccCrypto(), m_config->hash());
}

void OtPIRImpl::start()
{
    if (m_started)
    {
        PIR_LOG(WARNING) << LOG_DESC("The OT-PIR has already been started");
        return;
    }
    PIR_LOG(INFO) << LOG_DESC("Start the OT-PIR");
    m_started = true;

    // start a thread to execute task
    startWorking();

    m_thread = std::make_shared<std::thread>([&] {
        bcos::pthread_setThreadName("otPir_io_service");
        while (m_started)
        {
            try
            {
                m_ioService->run();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (m_started && m_ioService->stopped())
                {
                    m_ioService->restart();
                }
            }
            catch (std::exception& e)
            {
                FRONT_LOG(WARNING)
                    << LOG_DESC("Exception in OT-PIR Thread:") << boost::diagnostic_information(e);
            }
        }
        PIR_LOG(INFO) << "OT-PIR exit";
    });

    startPingTimer();
}

void OtPIRImpl::stop()
{
    if (!m_started)
    {
        return;
    }
    PIR_LOG(INFO) << LOG_DESC("Stop OT-PIR");
    m_started = false;

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

    if (m_senderThreadPool)
    {
        m_senderThreadPool->stop();
    }
    if (m_ioService)
    {
        m_ioService->stop();
    }
    // stop the thread
    if (m_thread->get_id() != std::this_thread::get_id())
    {
        m_thread->join();
    }
    else
    {
        m_thread->detach();
    }
    stopPingTimer();

    PIR_LOG(INFO) << LOG_DESC("OT-PIR stopped");
}

// register to the front to get the message related to cm2020-psi
void OtPIRImpl::onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg)
{
    try
    {
        m_msgQueue->push(_msg);

        // notify to handle the message
        m_signal.notify_all();
    }
    catch (std::exception const& e)
    {
        PIR_LOG(WARNING) << LOG_DESC("onReceiveMessage exception") << printPPCMsg(_msg)
                         << LOG_KV("error", boost::diagnostic_information(e));
    }
}

void OtPIRImpl::onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message)
{
    PIR_LOG(WARNING) << LOG_DESC("onReceivedErrorNotification") << printPPCMsg(_message);
    // finish the task while the peer is failed
    auto taskState = findPendingTask(_message->taskID());
    if (taskState)
    {
        taskState->onPeerNotifyFinish();

        wakeupWorker();
    }
}

void OtPIRImpl::onSelfError(const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer)
{
    PIR_LOG(WARNING) << LOG_DESC("onSelfError") << LOG_KV("task", _taskID)
                     << LOG_KV("error", _error->errorMessage())
                     << LOG_KV("noticePeer", _noticePeer);

    auto taskState = findPendingTask(_taskID);
    if (!taskState)
    {
        return;
    }

    auto result = std::make_shared<TaskResult>(taskState->task()->id());
    result->setError(std::move(_error));
    taskState->onTaskFinished(result, _noticePeer);

    wakeupWorker();
}


// ot-pir main processing function
// for ut to make this function public
void OtPIRImpl::executeWorker()
{
    checkFinishedTask();
    auto result = m_msgQueue->tryPop(c_popWaitMs);
    if (result.first)
    {
        handleReceivedMessage(result.second);
        return;
    }
    waitSignal();
}


void OtPIRImpl::checkFinishedTask()
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
            if (task->finished())
            {
                finishedTask.insert(it->first);
            }
            it++;
        }
    }
    for (auto& taskID : finishedTask)
    {
        removeReceiver(taskID);
        removeSender(taskID);
        removePendingTask(taskID);
        // asyncRunTask();
    }
}


void OtPIRImpl::handleReceivedMessage(const ppc::front::PPCMessageFace::Ptr& _message)
{
    PIR_LOG(TRACE) << LOG_DESC("handleReceivedMessage") << printPPCMsg(_message);

    m_config->threadPool()->enqueue([self = weak_from_this(), _message]() {
        try
        {
            auto pir = self.lock();
            if (!pir)
            {
                return;
            }
            switch (int(_message->messageType()))
            {
            case int(CommonMessageType::ErrorNotification):
            {
                pir->onReceivedErrorNotification(_message);
                break;
            }
            case int(CommonMessageType::PingPeer):
            {
                break;
            }
            case int(OTPIRMessageType::HELLO_RECEIVER):
            {
                pir->onHelloReceiver(_message);
                break;
            }
            case int(OTPIRMessageType::RESULTS):
            {
                pir->onSnederResults(_message);
                break;
            }
            default:
            {
                PIR_LOG(WARNING) << LOG_DESC("unsupported messageType ")
                                 << unsigned(_message->messageType());
                break;
            }
            }
        }
        catch (std::exception const& e)
        {
            PIR_LOG(WARNING) << LOG_DESC("handleReceivedMessage exception")
                             << LOG_KV("type", unsigned(_message->messageType()))
                             << printPPCMsg(_message)
                             << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}

void OtPIRImpl::onHelloReceiver(const ppc::front::PPCMessageFace::Ptr& _message)
{
    // 接收方不需要记录taskID
    // if (m_taskState->taskDone())
    // {
    //     return;
    // }
    PIR_LOG(DEBUG) << LOG_BADGE("onHelloReceiver") << LOG_KV("taskID", _message->taskID())
                   << LOG_KV("seq", _message->seq()) << LOG_KV("length", _message->length());
    ppctars::SenderMessageParams senderMessageParams;
    ppctars::serialize::decode(*_message->data(), senderMessageParams);
    // crypto::SenderMessage senderMessage;
    // TODO: how to find my dataset
    // m_taskState->setReader(io::LineReader::Ptr _reader, int64_t _readerParam)

    try
    {
        // auto writer = m_taskState->reader();
        auto receiver = findReceiver(_message->taskID());
        auto path = receiver.path;
        PIR_LOG(INFO) << LOG_BADGE("onHelloReceiver") << LOG_KV("taskID", _message->taskID())
                      << LOG_KV("requestAgencyDataset", path)
                      << LOG_KV("sendObfuscatedHash",
                             std::string(senderMessageParams.sendObfuscatedHash.begin(),
                                 senderMessageParams.sendObfuscatedHash.end()));

        auto messageKeypair = m_ot->prepareDataset(senderMessageParams.sendObfuscatedHash, path);
        auto receiverMessage = m_ot->receiverGenerateMessage(senderMessageParams.pointX,
            senderMessageParams.pointY, messageKeypair, senderMessageParams.pointZ);
        ppctars::ReceiverMessageParams receiverMessageParams;
        receiverMessageParams.encryptMessagePair = receiverMessage.encryptMessagePair;
        receiverMessageParams.encryptCipher = receiverMessage.encryptCipher;
        receiverMessageParams.pointWList = receiverMessage.pointWList;
        // PIR_LOG(INFO) << LOG_BADGE("buildPPCMessage");

        auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PIR),
            uint8_t(protocol::TaskAlgorithmType::OT_PIR_2PC), m_taskID,
            std::make_shared<bcos::bytes>());
        message->setMessageType(uint8_t(OTPIRMessageType::RESULTS));
        ppctars::serialize::encode(receiverMessageParams, *message->data());
        // PIR_LOG(INFO) << LOG_BADGE("asyncSendMessage");

        m_config->front()->asyncSendMessage(
            m_taskState->peerID(), message, m_config->networkTimeout(),
            [self = weak_from_this()](bcos::Error::Ptr _error) {
                auto receiver = self.lock();
                if (!receiver)
                {
                    return;
                }
                if (_error && _error->errorCode())
                {
                    receiver->onReceiverTaskDone(std::move(_error));
                }
            },
            nullptr);
        auto endTask = std::make_shared<TaskResult>(m_taskState->task()->id());
        m_taskState->onTaskFinished(endTask, true);
    }
    catch (bcos::Error const& e)
    {
        PIR_LOG(WARNING) << LOG_DESC("onHelloReceiver exception") << LOG_KV("code", e.errorCode())
                         << LOG_KV("msg", e.errorMessage());
        onSelfError(m_taskID, std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), true);
    }
}

void OtPIRImpl::onSnederResults(ppc::front::PPCMessageFace::Ptr _message)
{
    PIR_LOG(DEBUG) << LOG_BADGE("onSnederResults") << LOG_KV("taskID", _message->taskID())
                   << LOG_KV("seq", _message->seq());

    ppctars::ReceiverMessageParams receiverMessageParams;
    ppctars::serialize::decode(*_message->data(), receiverMessageParams);
    crypto::SenderMessage senderMessage = findSender(_message->taskID());
    PIR_LOG(DEBUG) << LOG_BADGE("onSnederResults")
                   << LOG_KV("scalarBlidingB", toHex(senderMessage.scalarBlidingB))
                   << LOG_KV("pointWList Size", receiverMessageParams.pointWList.size())
                   << LOG_KV("encryptCipher Size", receiverMessageParams.encryptCipher.size())
                   << LOG_KV("encryptMessagePair Size",
                          receiverMessageParams.encryptMessagePair.size());
    bcos::bytes result =
        m_ot->finishSender(senderMessage.scalarBlidingB, receiverMessageParams.pointWList,
            receiverMessageParams.encryptMessagePair, receiverMessageParams.encryptCipher);
    saveResults(result);
    auto endTask = std::make_shared<TaskResult>(m_taskState->task()->id());
    m_taskState->onTaskFinished(endTask, true);
}

void OtPIRImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, TaskResponseCallback&& _onTaskFinished)
{
    // TODO
    PIR_LOG(INFO) << LOG_DESC("receive a task") << LOG_KV("taskID", _task->id());
    m_taskID = _task->id();
    addTask(_task, [self = weak_from_this(), taskID = _task->id(), _onTaskFinished](
                       ppc::protocol::TaskResult::Ptr&& _result) {
        auto result = std::move(_result);
        _onTaskFinished(std::move(result));
        PIR_LOG(INFO) << LOG_DESC("finish a task") << LOG_KV("taskID", taskID);
        auto pir = self.lock();
        if (!pir)
        {
            return;
        }
        pir->m_parallelism++;
    });
    asyncRunTask();
}

void OtPIRImpl::asyncRunTask()
{
    PIR_LOG(INFO) << LOG_DESC("asyncRunTask") << LOG_KV("current semaphore", m_parallelism);

    if (m_parallelism <= 0)
    {
        return;
    }

    std::pair<ppc::protocol::Task::ConstPtr, TaskResponseCallback> taskPair;
    {
        bcos::UpgradableGuard l(x_taskQueue);
        if (m_taskQueue.empty())
        {
            return;
        }

        bcos::UpgradeGuard ul(l);
        taskPair = m_taskQueue.front();
        m_taskQueue.pop();
        m_parallelism--;
    }

    auto task = taskPair.first;

    bcos::Error::Ptr error;

    // add pending task
    m_taskState =
        m_taskStateFactory->createTaskState(task, std::move(taskPair.second), false, m_config);
    m_taskState->setPeerID(getPeerID(task));
    // m_taskState->registerNotifyPeerFinishHandler([self = weak_from_this(), task]() {
    //     auto pir = self.lock();
    //     if (!pir)
    //     {
    //         return;
    //     }

    //     pir->noticePeerToFinish(task);
    // });
    addPendingTask(m_taskState);

    try
    {
        auto dataResource = task->selfParty()->dataResource();
        auto role = task->selfParty()->partyIndex();

        if (role == uint16_t(PartyType::Client))
        {
            auto originData = m_taskState->task()->param();

            PIR_LOG(TRACE) << LOG_DESC("originData") << LOG_KV("originData", originData);
            PirTaskMessage taskMessage = parseJson(originData);
            PIR_LOG(TRACE) << LOG_DESC("taskMessage")
                           << LOG_KV("requestAgencyDataset", taskMessage.requestAgencyDataset)
                           << LOG_KV("prefixLength", taskMessage.prefixLength)
                           << LOG_KV("searchId", taskMessage.searchId);
            auto writer =
                loadWriter(task->id(), dataResource, m_taskState->task()->enableOutputExists());
            m_taskState->setWriter(writer);
            runSenderGenerateCipher(taskMessage);
        }
        else if (role == uint16_t(PartyType::Server))
        {
            // server接受任务请求，初始化reader
            PIR_LOG(TRACE) << LOG_DESC("Server init");
            crypto::ReceiverMessage receiverMessage;
            receiverMessage.path = dataResource->desc()->path();
            addReceiver(receiverMessage);
            // m_resource = dataResource;
            // auto reader = loadReader(task->id(), dataResource, DataSchema::Bytes);
            // m_taskState->setReader(reader, -1);
        }
        else
        {
            PIR_LOG(WARNING) << LOG_DESC("undefined task role") << unsigned(role);
            onSelfError(task->id(),
                std::make_shared<bcos::Error>(
                    (int)OTPIRRetCode::UNDEFINED_TASK_ROLE, "undefined task role"),
                true);
        }
    }
    catch (bcos::Error const& e)
    {
        PIR_LOG(WARNING) << LOG_DESC("asyncRunTask exception") << printTaskInfo(task)
                         << LOG_KV("code", e.errorCode()) << LOG_KV("msg", e.errorMessage());
        onSelfError(
            task->id(), std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), true);
    }
    catch (const std::exception& e)
    {
        PIR_LOG(WARNING) << LOG_DESC("asyncRunTask exception") << printTaskInfo(task)
                         << LOG_KV("error", boost::diagnostic_information(e));
        onSelfError(task->id(),
            std::make_shared<bcos::Error>((int)OTPIRRetCode::ON_EXCEPTION,
                "exception caught while running task: " + boost::diagnostic_information(e)),
            true);
    }

    // notify the taskInfo to the front
    error = m_config->front()->notifyTaskInfo(task->id());
    if (error && error->errorCode())
    {
        onSelfError(task->id(), error, true);
    }
}

void OtPIRImpl::runSenderGenerateCipher(PirTaskMessage taskMessage)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    crypto::SenderMessage senderMessage = m_ot->senderGenerateCipher(
        bcos::bytes(taskMessage.searchId.begin(), taskMessage.searchId.end()),
        taskMessage.prefixLength);
    ppctars::SenderMessageParams senderMessageParams;
    senderMessageParams.pointX = senderMessage.pointX;
    senderMessageParams.pointY = senderMessage.pointY;
    senderMessageParams.pointZ = senderMessage.pointZ;
    // senderMessageParams.requestAgencyDataset = taskMessage.requestAgencyDataset;
    senderMessageParams.sendObfuscatedHash = senderMessage.sendObfuscatedHash;
    auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PIR),
        uint8_t(protocol::TaskAlgorithmType::OT_PIR_2PC), m_taskID,
        std::make_shared<bcos::bytes>());
    message->setMessageType(uint8_t(OTPIRMessageType::HELLO_RECEIVER));
    ppctars::serialize::encode(senderMessageParams, *message->data());
    addSender(senderMessage);
    // PIR_LOG(INFO) << LOG_BADGE("runSenderGenerateCipher") << LOG_KV("taskID", m_taskID) <<
    // LOG_KV("requestAgencyDataset", senderMessageParams.requestAgencyDataset);
    PIR_LOG(INFO) << LOG_BADGE("runSenderGenerateCipher") << LOG_KV("taskID", m_taskID)
                  << LOG_KV("sendObfuscatedHash",
                         std::string(senderMessageParams.sendObfuscatedHash.begin(),
                             senderMessageParams.sendObfuscatedHash.end()));
    // senderMessageParams.taskId = m_taskID;
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](bcos::Error::Ptr _error) {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                receiver->onReceiverTaskDone(std::move(_error));
            }
        },
        nullptr);
}


void OtPIRImpl::onReceiverTaskDone(bcos::Error::Ptr _error)
{
    if (m_taskState->taskDone() && (!_error || _error->errorCode() == 0))
    {
        return;
    }

    PIR_LOG(INFO) << LOG_BADGE("onReceiverTaskDone") LOG_KV("taskID", m_taskID);

    std::string message;
    if (_error)
    {
        message = "\nStatus: FAIL\nMessage: " + _error->errorMessage();
        m_taskResult->setError(std::move(_error));
    }
    m_taskState->onTaskFinished(m_taskResult, true);

    PIR_LOG(INFO) << LOG_BADGE("receiverTaskDone") << LOG_KV("taskID", m_taskID)
                  << LOG_KV("detail", message);
}


void OtPIRImpl::saveResults(bcos::bytes result)
{
    PIR_LOG(INFO) << LOG_BADGE("saveResults") LOG_KV("taskID", m_taskID);
    try
    {
        DataBatch::Ptr finalResults = std::make_shared<DataBatch>();
        finalResults->append<bcos::bytes>(result);
        m_taskState->writeLines(finalResults, DataSchema::Bytes);
    }
    catch (const std::exception& e)
    {
        PIR_LOG(WARNING) << LOG_KV("taskID", m_taskID)
                         << LOG_KV("error", boost::diagnostic_information(e));
        auto error = std::make_shared<bcos::Error>(
            (int)OTPIRRetCode::ON_EXCEPTION, boost::diagnostic_information(e));
        onReceiverTaskDone(error);
    }
}
