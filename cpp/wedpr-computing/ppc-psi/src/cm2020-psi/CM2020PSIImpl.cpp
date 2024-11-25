/**
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
 * @file CM2020PSIImpl.cpp
 * @author: shawnhe
 * @date 2022-12-7
 */

#include "CM2020PSIImpl.h"
#include "Common.h"
#include "ppc-tools/src/common/TransTools.h"

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::tools;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::task;

CM2020PSIImpl::CM2020PSIImpl(const CM2020PSIConfig::Ptr& _config, unsigned _idleTimeMs)
  : Worker("CM2020-PSI", _idleTimeMs),
    TaskGuarder(_config, TaskAlgorithmType::CM_PSI_2PC, "CM2020-PSI-Timer"),
    m_config(_config),
    m_msgQueue(std::make_shared<CM2020PSIMsgQueue>()),
    m_ioService(std::make_shared<boost::asio::io_service>()),
    m_parallelism(m_config->parallelism())
{
    m_threadPool = std::make_shared<bcos::ThreadPool>(
        "CM2020-PSI-ThreadPool", static_cast<uint32_t>(std::thread::hardware_concurrency() * 0.75));
    m_ot = std::make_shared<SimplestOT>(m_config->eccCrypto(), m_config->hash());
}

void CM2020PSIImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, TaskResponseCallback&& _onTaskFinished)
{
    CM2020_PSI_LOG(INFO) << LOG_DESC("receive a task") << LOG_KV("taskID", _task->id());
    addTask(_task, [self = weak_from_this(), taskID = _task->id(), _onTaskFinished](
                       ppc::protocol::TaskResult::Ptr&& _result) {
        try
        {
            auto result = std::move(_result);
            _onTaskFinished(std::move(result));
            CM2020_PSI_LOG(INFO) << LOG_DESC("finish a task") << LOG_KV("taskID", taskID);
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            psi->m_parallelism++;

            auto timer = std::make_shared<boost::asio::deadline_timer>(
                *(psi->m_ioService), boost::posix_time::minutes(WAITING_PEER_FINISH_M));
            timer->async_wait([self, timer, taskID](boost::system::error_code) {
                auto psi = self.lock();
                if (!psi)
                {
                    return;
                }
                // erase the taskInfo from the gateway
                CM2020_PSI_LOG(INFO) << LOG_DESC("erase task info") << LOG_KV("taskID", taskID);
                psi->m_config->front()->eraseTaskInfo(taskID);
            });
        }
        catch (std::exception& e)
        {
            CM2020_PSI_LOG(ERROR) << LOG_DESC("handle callback error after finishing task")
                                  << LOG_KV("exception", boost::diagnostic_information(e));
        }
    });

    m_threadPool->enqueue([self = weak_from_this()]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        try
        {
            psi->asyncRunTask();
        }
        catch (std::exception& e)
        {
            CM2020_PSI_LOG(ERROR) << LOG_DESC("asyncRunTask error")
                                  << LOG_KV("exception", boost::diagnostic_information(e));
        }
    });
}

// run task
void CM2020PSIImpl::asyncRunTask()
{
    CM2020_PSI_LOG(INFO) << LOG_DESC("asyncRunTask") << LOG_KV("current semaphore", m_parallelism);

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

    // check task
    auto error = checkTask(task, 2, true, task->syncResultToPeer(), false);
    if (error)
    {
        CM2020_PSI_LOG(ERROR) << LOG_DESC("failed to check task, " + error->errorMessage())
                              << printTaskInfo(task);
        // notice peer to cancel task
        noticePeerToFinish(task);
        auto result = std::make_shared<TaskResult>(task->id());
        result->setError(std::move(error));
        taskPair.second(std::move(result));

        // mark this taskID as occupied
        m_config->front()->notifyTaskInfo(task->id());
        return;
    }

    // add pending task
    auto taskState =
        m_taskStateFactory->createTaskState(task, std::move(taskPair.second), false, m_config);
    taskState->setPeerID(getPeerID(task));
    taskState->registerNotifyPeerFinishHandler([self = weak_from_this(), task]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        CM2020_PSI_LOG(INFO) << LOG_DESC("noticePeerToFinish") << printTaskInfo(task);
        psi->noticePeerToFinish(task);
    });
    try
    {
        addPendingTask(taskState);
        // check the memory
        checkHostResource(m_config->minNeededMemoryGB());
        // prepare reader and writer
        auto dataResource = task->selfParty()->dataResource();
        auto reader = loadReader(task->id(), dataResource, DataSchema::Bytes);
        taskState->setReader(reader, -1);

        auto role = task->selfParty()->partyIndex();
        if (role == uint16_t(PartyType::Client) || task->syncResultToPeer())
        {
            auto writer = loadWriter(task->id(), dataResource, task->enableOutputExists());
            taskState->setWriter(writer);
        }

        if (role == uint16_t(PartyType::Client))
        {
            auto receiver = std::make_shared<CM2020PSIReceiver>(m_config, taskState, m_ot);
            receiver->asyncRunTask();
            addReceiver(std::move(receiver));
        }
        else if (role == uint16_t(PartyType::Server))
        {
            auto sender = std::make_shared<CM2020PSISender>(
                m_config, taskState, m_ioService, m_ot, m_threadPool);
            sender->asyncRunTask();
            addSender(std::move(sender));
        }
        else
        {
            CM2020_PSI_LOG(ERROR) << LOG_DESC("undefined task role") << unsigned(role);
            onSelfError(task->id(),
                std::make_shared<bcos::Error>(
                    (int)CM2020PSIRetCode::UNDEFINED_TASK_ROLE, "undefined task role"),
                true);
        }
    }
    catch (bcos::Error const& e)
    {
        CM2020_PSI_LOG(ERROR) << LOG_DESC("asyncRunTask exception") << printTaskInfo(task)
                              << LOG_KV("code", e.errorCode()) << LOG_KV("msg", e.errorMessage());
        onSelfError(
            task->id(), std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), true);
    }
    catch (const std::exception& e)
    {
        CM2020_PSI_LOG(ERROR) << LOG_DESC("asyncRunTask exception") << printTaskInfo(task)
                              << LOG_KV("exception", boost::diagnostic_information(e));
        onSelfError(task->id(),
            std::make_shared<bcos::Error>((int)CM2020PSIRetCode::ON_EXCEPTION,
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

// register to the front to get the message related to cm2020-psi
void CM2020PSIImpl::onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg)
{
    try
    {
        m_msgQueue->push(_msg);

        // notify to handle the message
        wakeupWorker();
    }
    catch (std::exception const& e)
    {
        CM2020_PSI_LOG(WARNING) << LOG_DESC("onReceiveMessage exception") << printPPCMsg(_msg)
                                << LOG_KV("exception", boost::diagnostic_information(e));
    }
}

void CM2020PSIImpl::start()
{
    if (m_started)
    {
        CM2020_PSI_LOG(ERROR) << LOG_DESC("The CM2020-PSI has already been started");
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_DESC("Start the CM2020-PSI");
    m_started = true;

    // start a thread to execute task
    startWorking();

    m_thread = std::make_shared<std::thread>([&] {
        bcos::pthread_setThreadName("cm2020_io_service");
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
                FRONT_LOG(WARNING) << LOG_DESC("Exception in CM2020-PSI Thread:")
                                   << boost::diagnostic_information(e);
            }
        }
        CM2020_PSI_LOG(INFO) << "CM2020-PSI exit";
    });

    startPingTimer();
}

void CM2020PSIImpl::stop()
{
    if (!m_started)
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_DESC("Stop CM2020-PSI");
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

    if (m_threadPool)
    {
        m_threadPool->stop();
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

    CM2020_PSI_LOG(INFO) << LOG_DESC("CM2020-PSI stopped");
}

void CM2020PSIImpl::onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message)
{
    CM2020_PSI_LOG(INFO) << LOG_DESC("onReceivedErrorNotification") << printPPCMsg(_message);
    // finish the task while the peer is failed
    auto taskState = findPendingTask(_message->taskID());
    if (taskState)
    {
        taskState->onPeerNotifyFinish();

        wakeupWorker();
    }
}

void CM2020PSIImpl::onSelfError(
    const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer)
{
    auto taskState = findPendingTask(_taskID);
    if (!taskState)
    {
        return;
    }

    CM2020_PSI_LOG(ERROR) << LOG_DESC("onSelfError") << LOG_KV("task", _taskID)
                          << LOG_KV("exception", _error->errorMessage())
                          << LOG_KV("noticePeer", _noticePeer);

    auto result = std::make_shared<TaskResult>(taskState->task()->id());
    result->setError(std::move(_error));
    taskState->onTaskFinished(result, _noticePeer);

    wakeupWorker();
}

// cm2020-psi main processing function
// for ut to make this function public
void CM2020PSIImpl::executeWorker()
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

void CM2020PSIImpl::checkFinishedTask()
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
        asyncRunTask();
    }
}

void CM2020PSIImpl::handleReceivedMessage(const ppc::front::PPCMessageFace::Ptr& _message)
{
    CM2020_PSI_LOG(TRACE) << LOG_DESC("handleReceivedMessage") << printPPCMsg(_message);

    m_config->threadPool()->enqueue([self = weak_from_this(), _message]() {
        try
        {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            switch (int(_message->messageType()))
            {
            case int(CommonMessageType::ErrorNotification):
            {
                psi->onReceivedErrorNotification(_message);
                break;
            }
            case int(CommonMessageType::PingPeer):
            {
                break;
            }
            case int(CM2020PSIMessageType::HELLO_RECEIVER):
            {
                psi->onReceiveHelloReceiver(_message);
                break;
            }
            case int(CM2020PSIMessageType::HELLO_SENDER):
            {
                psi->onReceiveHelloSender(_message);
                break;
            }
            case int(CM2020PSIMessageType::RECEIVER_SIZE):
            {
                psi->onReceiveReceiverSize(_message);
                break;
            }
            case int(CM2020PSIMessageType::SENDER_SIZE):
            {
                psi->onReceiveSenderSize(_message);
                break;
            }
            case int(CM2020PSIMessageType::POINT_A):
            {
                psi->onReceivePointA(_message);
                break;
            }
            case int(CM2020PSIMessageType::POINT_B_ARRAY):
            {
                psi->onReceiveBatchPointB(_message);
                break;
            }
            case int(CM2020PSIMessageType::MATRIX):
            {
                psi->onReceiveMatrix(_message);
                break;
            }
            case int(CM2020PSIMessageType::DO_NEXT_ROUND):
            {
                psi->onReceiveDoNextRound(_message);
                break;
            }
            case int(CM2020PSIMessageType::HASHES):
            {
                psi->onReceiveHashes(_message);
                break;
            }
            case int(CM2020PSIMessageType::RESULTS_SIZE):
            {
                psi->onReceiveResultCount(_message);
                break;
            }
            case int(CM2020PSIMessageType::RESULTS):
            {
                psi->onReceiveResults(_message);
                break;
            }
            default:
            {
                CM2020_PSI_LOG(WARNING)
                    << LOG_DESC("unsupported messageType ") << unsigned(_message->messageType());
                break;
            }
            }
        }
        catch (std::exception const& e)
        {
            CM2020_PSI_LOG(WARNING)
                << LOG_DESC("handleReceivedMessage exception")
                << LOG_KV("type", unsigned(_message->messageType())) << printPPCMsg(_message)
                << LOG_KV("exception", boost::diagnostic_information(e));
        }
    });
}


void CM2020PSIImpl::onReceiveHelloReceiver(const ppc::front::PPCMessageFace::Ptr& _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->onHandshakeDone();
    }
}

void CM2020PSIImpl::onReceiveHelloSender(const ppc::front::PPCMessageFace::Ptr& _message)
{
    auto sender = findSender(_message->taskID());
    if (sender)
    {
        sender->onHandshakeDone(_message);
    }
}

void CM2020PSIImpl::onReceiveReceiverSize(ppc::front::PPCMessageFace::Ptr _message)
{
    auto sender = findSender(_message->taskID());
    if (sender)
    {
        sender->onReceiverSizeReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceiveSenderSize(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->onSenderSizeReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceivePointA(ppc::front::PPCMessageFace::Ptr _message)
{
    auto sender = findSender(_message->taskID());
    if (sender)
    {
        sender->onPointAReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceiveBatchPointB(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->onBatchPointBReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceiveMatrix(ppc::front::PPCMessageFace::Ptr _message)
{
    auto sender = findSender(_message->taskID());
    if (sender)
    {
        sender->onMatrixColumnReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceiveDoNextRound(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->onDoNextRoundReceived();
    }
}

void CM2020PSIImpl::onReceiveHashes(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->onHashesReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceiveResultCount(ppc::front::PPCMessageFace::Ptr _message)
{
    auto sender = findSender(_message->taskID());
    if (sender)
    {
        sender->onResultCountReceived(std::move(_message));
    }
}

void CM2020PSIImpl::onReceiveResults(ppc::front::PPCMessageFace::Ptr _message)
{
    auto sender = findSender(_message->taskID());
    if (sender)
    {
        sender->onResultReceived(std::move(_message));
    }
}
