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
 * @file PSIFramework.cpp
 * @author: yujiechen
 * @date 2022-12-26
 */
#include "PSIFramework.h"
#include "../Common.h"
#include "ppc-framework/protocol/Constant.h"
#include "ppc-framework/protocol/GlobalConfig.h"

using namespace ppc::task;
using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace bcos;
using namespace ppc::front;

void PSIFramework::start()
{
    if (m_started)
    {
        PSI_FRAMEWORK_LOG(WARNING) << LOG_DESC("The PSI has already been started");
        return;
    }
    m_started = true;
    m_taskSyncTimer->registerTimeoutHandler([this]() { syncTaskInfo(); });
    m_taskSyncTimer->start();
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("Start the PSI");
    // start a thread to execute task
    startWorking();
}

void PSIFramework::stop()
{
    if (m_started == false)
    {
        return;
    }
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("Stop PSI");
    m_started = false;
    if (m_taskSyncTimer)
    {
        m_taskSyncTimer->stop();
    }
    finishWorker();
    if (isWorking())
    {
        // stop the worker thread
        stopWorking();
        terminate();
    }
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("PSI stopped");
}

void PSIFramework::executeWorker()
{
    handleLocalTask();
    checkAndNotifyTaskResult();
    auto result = m_msgQueue->tryPop(c_PopWaitMs);
    if (result.first)
    {
        handlePSIMsg(result.second);
        return;
    }
    waitSignal();
}

// handle the local task
void PSIFramework::handleLocalTask()
{
    bcos::ReadGuard l(x_pendingTasks);
    if (m_pendingTasks.empty())
    {
        return;
    }
    for (auto const& it : m_pendingTasks)
    {
        if (it.second->finished())
        {
            continue;
        }
        // Note: executeWork should not occupy too-much time
        it.second->executeWork();
    }
}


// check the task-finished-or-not and response the result of the finished task
void PSIFramework::checkAndNotifyTaskResult()
{
    std::set<std::string> erasedResourceID;
    {
        bcos::UpgradableGuard l(x_pendingTasks);
        // with-no-task
        if (m_pendingTasks.empty())
        {
            return;
        }
        for (auto it = m_pendingTasks.begin(); it != m_pendingTasks.end();)
        {
            auto taskState = it->second;
            if (!taskState->finished())
            {
                it++;
                continue;
            }
            // notify the result to peer
            auto dataResource = (taskState->task()->selfParty()->dataResource());
            if (dataResource)
            {
                erasedResourceID.insert(dataResource->resourceID());
            }
            {
                bcos::UpgradeGuard ul(l);
                it = m_pendingTasks.erase(it);
            }
            notifyTaskResult(nullptr, taskState->peerID(), taskState->task()->id(),
                dataResource ? dataResource->resourceID() : "");
            // notify the result to rpc response
            taskState->onTaskFinished();
        }
    }
    // clear the dataResourceInfo related to the finished-task
    batchRemoveDataResource(erasedResourceID);
}

// Note: we should not block this function too much time
void PSIFramework::onReceiveMessage(PPCMessageFace::Ptr _msg)
{
    try
    {
        // decode the psi message and put it into to m_msgQueue
        auto payLoad = _msg->data();
        auto psiMsg =
            m_msgFactory->decodePSIMessage(bcos::bytesConstRef(payLoad->data(), payLoad->size()));
        psiMsg->setFrom(_msg->sender());
        psiMsg->setTaskID(_msg->taskID());
        psiMsg->setSeq(_msg->seq());
        psiMsg->setUUID(_msg->uuid());
        psiMsg->setFromNode(_msg->senderNode());
        m_msgQueue->push(psiMsg);
        PSI_FRAMEWORK_LOG(TRACE) << LOG_DESC("onReceiveMessage") << printPSIMessage(psiMsg)
                                 << LOG_KV("uuid", _msg->uuid());
        // release the large payload immediately
        if (payLoad && payLoad->size() >= ppc::protocol::LARGE_MSG_THRESHOLD)
        {
            PSI_FRAMEWORK_LOG(INFO)
                << LOG_DESC("Release large message payload") << LOG_KV("size", payLoad->size());
            _msg->releasePayload();
            MallocExtension::instance()->ReleaseFreeMemory();
        }
        // notify to handle the message
        m_signalled.notify_all();
    }
    catch (std::exception const& e)
    {
        PSI_FRAMEWORK_LOG(WARNING) << LOG_DESC("onReceiveMessage exception") << printPPCMsg(_msg)
                                   << LOG_KV("error", boost::diagnostic_information(e))
                                   << LOG_KV("msgSize", _msg->data()->size());
    }
}

void PSIFramework::onTaskError(std::string const& _desc, bcos::Error::Ptr&& _error,
    std::string const& _peerID, std::string const& _taskID, std::string const& _resourceID)
{
    if (!_error)
    {
        PSI_FRAMEWORK_LOG(DEBUG) << LOG_DESC(_desc) << LOG_KV("peer", _peerID)
                                 << LOG_KV("task", _taskID) << LOG_KV("resource", _resourceID);
        return;
    }
    PSI_FRAMEWORK_LOG(WARNING) << LOG_DESC(_desc) << " error" << LOG_KV("peer", _peerID)
                               << LOG_KV("task", _taskID) << LOG_KV("resource", _resourceID)
                               << LOG_KV("code", _error->errorCode())
                               << LOG_KV("msg", _error->errorMessage());
    // notify the peer that the task has been canceled
    notifyTaskResult(_error, _peerID, _taskID, _resourceID);
    // cancel the task
    cancelTask(std::move(_error), _taskID);
}


// cancel the task and response to the user when error happens
void PSIFramework::cancelTask(bcos::Error::Ptr&& _error, std::string const& _task)
{
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("cancelTask") << LOG_KV("task", _task)
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
    TaskResponseCallback callback = nullptr;
    std::function<void()> finalizeCallback = nullptr;
    TaskState::Ptr taskState = nullptr;
    {
        // get and delete the pending-tasks
        UpgradableGuard l(x_pendingTasks);
        auto it = m_pendingTasks.find(_task);
        // find in the pending-task
        if (it != m_pendingTasks.end())
        {
            taskState = it->second;
            UpgradeGuard ul(l);
            // take the callback in-case-of the callback been called more than once
            callback = it->second->takeCallback();
            finalizeCallback = it->second->takeFinalizeHandler();
            // erase from the pending-tasks
            m_pendingTasks.erase(it);
        }
    }
    if (taskState && taskState->task()->selfParty()->dataResource())
    {
        removeLockingResource(taskState->task()->selfParty()->dataResource()->resourceID());
    }
    {
        PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("cancelTask: response task result")
                                << LOG_KV("task", _task) << LOG_KV("code", _error->errorCode())
                                << LOG_KV("msg", _error->errorMessage());
        auto taskResult = std::make_shared<TaskResult>(_task);
        taskResult->setError(_error);
        try
        {
            // calls the finalizeCallback to finalize when task failed
            // make sure the resource has been released when the task failed
            if (finalizeCallback)
            {
                finalizeCallback();
            }

            if (taskState)
            {
                auto reader = taskState->reader();
                if (reader)
                {
                    reader->clean();
                }

                auto writer = taskState->writer();
                if (writer)
                {
                    writer->clean();
                }

                if (writer && !_error->errorCode())
                {
                    writer->upload();
                }
            }
        }
        catch (std::exception const& e)
        {
            PSI_LOG(WARNING) << LOG_DESC("cancelTask exception")
                             << LOG_KV("msg", boost::diagnostic_information(e));
            auto error = std::make_shared<bcos::Error>(-1, boost::diagnostic_information(e));
            taskResult->setError(std::move(error));
        }
        if (callback)
        {
            callback(std::move(taskResult));
        }
    }
}

bool PSIFramework::checkDataResourceForSelf(
    TaskState::Ptr const& _taskState, std::string const& _peerID, bool _requireOutput)
{
    auto task = _taskState->task();
    auto dataResource = task->selfParty()->dataResource();
    // check the dataResource
    if (!dataResource || !dataResource->desc())
    {
        std::string errorMessage = "runPSI failed: Must specified the input data-resource";
        auto error =
            std::make_shared<Error>((int)PSIRetCode::NotSpecifyInputDataResource, errorMessage);
        onTaskError("runPSI", std::move(error), _peerID, task->id(),
            dataResource ? dataResource->resourceID() : "empty");
        PSI_LOG(WARNING) << LOG_DESC("runPSI error: ") << errorMessage;
        return false;
    }
    if (_requireOutput && !dataResource->outputDesc())
    {
        std::string errorMessage = "Must specified the output data-resource";
        auto error =
            std::make_shared<Error>((int)PSIRetCode::NotSpecifyOutputDataResource, errorMessage);
        onTaskError("runPSI", std::move(error), _peerID, task->id(), dataResource->resourceID());
        PSI_LOG(WARNING) << LOG_DESC("runPSI error: ") << errorMessage;
        return false;
    }
    return true;
}

bcos::Error::Ptr PSIFramework::lockResourceAndRecordTaskState(
    int _command, TaskState::Ptr const& _taskState)
{
    // should notify the worker to check the task-status when the sub-task finished
    _taskState->registerSubTaskFinishedHandler([this]() { wakeupWorker(); });
    auto const& task = _taskState->task();
    bcos::Error::Ptr error = nullptr;
    {
        // check the task-in-progressing or not
        bcos::UpgradableGuard l(x_pendingTasks);
        if (m_pendingTasks.count(task->id()))
        {
            error = BCOS_ERROR_PTR(
                (int)PSIRetCode::TaskInProcessing, "The task " + task->id() + " is in-processing!");
        }
        else
        {
            // insert the task into pending-task-pool
            bcos::UpgradeGuard ul(l);
            m_pendingTasks[task->id()] = _taskState;
        }
    }
    auto ret = error;
    if (error)
    {
        // trigger the callback
        // Note: here can not calls cancel task for cancelTask will cancel the processing task
        auto taskResult = std::make_shared<TaskResult>(task->id());
        PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("lockResourceAndRecordTaskState: cancel task")
                                << LOG_KV("task", task) << LOG_KV("code", error->errorCode())
                                << LOG_KV("msg", error->errorMessage());
        taskResult->setError(std::move(error));
        auto callback = _taskState->takeCallback();
        callback(std::move(taskResult));
        return ret;
    }
    // Note: not all type tasks has resourceID
    auto partyIndex = task->selfParty()->partyIndex();
    auto const& dataResource = task->selfParty()->dataResource();
    auto resourceID = dataResource ? dataResource->resourceID() : "empty";
    if (dataResource && needLockResource(_command, partyIndex))
    {
        // Note: can't process the same data-resource in the same time
        bcos::UpgradableGuard l(x_processingDataResource);
        if (m_processingDataResource.count(resourceID))
        {
            error = BCOS_ERROR_PTR((int)PSIRetCode::DataResourceOccupied,
                "The dataResource " + resourceID + " is in-processing!");
        }
        else
        {
            bcos::UpgradeGuard ul(l);
            m_processingDataResource.insert(resourceID);
        }
    }
    // Note: cancelTask will try to occupy lock x_processingDataResource and x_pendingTasks
    // we should be careful when use cancelTask with locking x_processingDataResource or
    // x_pendingTasks
    ret = error;
    if (error)
    {
        cancelTask(std::move(error), task->id());
        return ret;
    }
    return nullptr;
}

ppc::protocol::PartyResource::Ptr PSIFramework::checkAndSetPeerInfo(
    TaskState::Ptr const& _taskState, bool _enforcePeerResource)
{
    auto const& task = _taskState->task();
    auto const& dataResource = task->selfParty()->dataResource();
    // check the peer
    auto const& peerParties = task->getAllPeerParties();
    if (peerParties.size() != 1)
    {
        std::string errorMessage =
            "PSI only support two-party, please limit the party to 1, passed party count "
            "is " +
            std::to_string(peerParties.size());
        auto error = std::make_shared<Error>((int)PSIRetCode::OnlySupportOnePeer, errorMessage);
        cancelTask(std::move(error), task->id());
        PSI_FRAMEWORK_LOG(WARNING)
            << LOG_DESC("runPSI error for invalid peer ") << errorMessage << printTaskInfo(task);
        return nullptr;
    }
    auto peerParty = peerParties.begin()->second;
    _taskState->setPeerID(peerParty->id());
    if (!_enforcePeerResource)
    {
        return peerParty;
    }
    // check the peerDataResource
    if (!peerParty->dataResource())
    {
        std::string errorMessage = "runPSI failed: Must specified the peer data-resource-id";
        auto error =
            std::make_shared<Error>((int)PSIRetCode::NotSpecifyPeerDataResource, errorMessage);
        onTaskError(
            "runPSI", std::move(error), peerParty->id(), task->id(), dataResource->resourceID());
        PSI_FRAMEWORK_LOG(WARNING) << LOG_DESC("runPSI error: ") << errorMessage;
        return nullptr;
    }
    return peerParty;
}

bool PSIFramework::checkPSIMsg(PSIMessageInterface::Ptr const& _msg)
{
    if (_msg->packetType() != (int)PSIPacketType::CancelTaskNotification &&
        _msg->packetType() != (int)PSIPacketType::TaskSyncMsg)
    {
        ReadGuard l(x_pendingTasks);
        if (!m_pendingTasks.count(_msg->taskID()))
        {
            PSI_FRAMEWORK_LOG(WARNING)
                << LOG_DESC("The task is not in the pendingPool") << printPSIMessage(_msg);
            // notify the peer the task canceled
            auto error = std::make_shared<Error>((int)PSIRetCode::TaskNotFound,
                "Task " + _msg->taskID() + " not found in " + m_psiConfig->selfParty() + " !");
            notifyTaskResult(error, _msg->from(), _msg->taskID(), _msg->resourceID());
            return false;
        }
    }
    return true;
}

// notify the peer the task has been canceled for some error
void PSIFramework::notifyTaskResult(bcos::Error::Ptr const& _error, std::string const& _peerID,
    std::string const& _taskID, std::string const& _resourceID)
{
    if (_peerID.empty() || _taskID.empty())
    {
        return;
    }
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("notifyTaskResult") << LOG_KV("peer", _peerID)
                            << LOG_KV("taskID", _taskID);
    auto taskNotificationMsg = m_msgFactory->createTaskNotificationMessage(
        (uint32_t)PSIPacketType::CancelTaskNotification);
    taskNotificationMsg->setResourceID(_resourceID);
    taskNotificationMsg->setErrorCode(_error ? _error->errorCode() : 0);
    taskNotificationMsg->setErrorMessage(_error ? _error->errorMessage() : "success");
    taskNotificationMsg->setTaskID(_taskID);
    taskNotificationMsg->setFrom(m_psiConfig->selfParty());
    m_psiConfig->generateAndSendPPCMessage(_peerID, _taskID, taskNotificationMsg,
        [this, taskNotificationMsg](bcos::Error::Ptr&& _error) {
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            PSI_FRAMEWORK_LOG(WARNING)
                << LOG_DESC("notifyTaskResult send error") << printPSIMessage(taskNotificationMsg)
                << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
        });
}


// broadcast the sync-task-information to all nodes of the given peerID
void PSIFramework::broadcastSyncTaskInfo(
    std::string const& _peerID, std::vector<std::string> const& _taskList)
{
    // Note: the empty task-id means that broadcast the message to all nodes
    std::string targetTaskID = "";
    auto taskSyncMsg = m_msgFactory->createTaskInfoMessage((uint32_t)PSIPacketType::TaskSyncMsg);
    taskSyncMsg->setTaskList(_taskList);
    // the partyID to using to differentiate psi nodes
    taskSyncMsg->setPartyID(m_psiConfig->front()->selfEndPoint());
    // broadcast the taskSyncMsg to all nodes
    m_psiConfig->generateAndSendPPCMessage(
        _peerID, targetTaskID, taskSyncMsg, [this, _peerID, _taskList](bcos::Error::Ptr&& _error) {
            if (!_error)
            {
                return;
            }
            //            PSI_FRAMEWORK_LOG(WARNING)
            //                << LOG_DESC(
            //                       "broadcastSyncTaskInfo: send message error, cancel all tasks
            //                       related to the " "peer")
            //                << LOG_KV("peer", _peerID) << LOG_KV("tasks", _taskList.size())
            //                << LOG_KV("code", _error->errorCode()) << LOG_KV("msg",
            //                _error->errorMessage());
            // cancel all the tasks
            for (auto const& task : _taskList)
            {
                auto error = BCOS_ERROR_PTR((int)PSIRetCode::TaskNotFound,
                    "Cancel task " + task + " for network error between peer-party!");
                PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("syncTaskInfo: cancel tasks for network error")
                                        << LOG_KV("task", task);
                cancelTask(std::move(error), task);
            }
        });
}

// sync the task-information to the peers periodically
void PSIFramework::syncTaskInfo()
{
    // get the pendingTasks
    std::map<std::string, std::vector<std::string>> pendingTasks;
    std::set<std::string> syncedPeers;
    {
        bcos::ReadGuard l(x_pendingTasks);
        for (auto const& it : m_pendingTasks)
        {
            auto const& peer = it.second->peerID();
            if (peer.empty())
            {
                continue;
            }
            pendingTasks[peer].emplace_back(it.first);
            syncedPeers.insert(peer);
        }
        PSI_FRAMEWORK_LOG(TRACE) << LOG_DESC("syncTaskInfo")
                                 << LOG_KV("totalPendingTasks", m_pendingTasks.size());
    }
    ////// sync the pending-tasks information to the peer
    // sync empty taskList to the peers without local-task corresponding-to
    auto agencyList = m_psiConfig->agencyList();
    for (auto const& it : agencyList)
    {
        if (syncedPeers.count(it))
        {
            continue;
        }
        if (it == m_psiConfig->selfParty())
        {
            continue;
        }
        PSI_FRAMEWORK_LOG(TRACE) << LOG_DESC("syncTaskInfo: sync empty task") << LOG_KV("peer", it);
        std::vector<std::string> emptyTaskList;
        broadcastSyncTaskInfo(it, emptyTaskList);
    }

    // sync tasks to peers with pendingTasks corresponding-to
    for (auto const& it : pendingTasks)
    {
        auto peerID = it.first;
        if (peerID.empty())
        {
            continue;
        }
        auto const& taskList = it.second;
        PSI_FRAMEWORK_LOG(TRACE) << LOG_DESC("syncTaskInfo")
                                 << LOG_KV("pendingTasks", taskList.size())
                                 << LOG_KV("peer", peerID);
        broadcastSyncTaskInfo(peerID, taskList);
    }
    m_taskSyncTimer->restart();
}

// receive the taskSync message, delete tasks that no longer exist in-peer
void PSIFramework::handleTaskSyncInfo(PSIMessageInterface::Ptr _msg)
{
    auto taskInfoMsg = std::dynamic_pointer_cast<PSITaskInfoMsg>(_msg);
    auto const& taskList = taskInfoMsg->taskList();
    std::set<std::string> peerTaskSet(taskList.begin(), taskList.end());
    updatePeerTasks(_msg->from(), _msg->partyID(), peerTaskSet);
    // get the local task list corresponding to the peer
    std::vector<std::string> localExpiredTaskList;
    {
        bcos::ReadGuard l(x_pendingTasks);
        for (auto const& it : m_pendingTasks)
        {
            auto const& peerID = it.second->peerID();
            if (peerID != _msg->from())
            {
                continue;
            }
            // only add the expired task to the localExpiredTaskList
            if (it.second->taskPendingTime() <= (uint64_t)m_psiConfig->taskExpireTime())
            {
                continue;
            }
            localExpiredTaskList.emplace_back(it.first);
        }
    }
    // compare and erase the task
    std::vector<std::string> tasksToCancel;
    auto peerTasks = getPeerTasks(_msg->from());
    for (auto const& it : localExpiredTaskList)
    {
        bool taskExists = false;
        for (auto const& taskSet : peerTasks)
        {
            if (taskSet.second.count(it))
            {
                taskExists = true;
                break;
            }
        }
        if (!taskExists)
        {
            tasksToCancel.emplace_back(it);
        }
    }
    PSI_FRAMEWORK_LOG(TRACE) << LOG_DESC("handleTaskSyncInfo") << printPSIMessage(_msg)
                             << LOG_KV("peerTaskSize", taskList.size())
                             << LOG_KV("localExpiredTaskList", localExpiredTaskList.size())
                             << LOG_KV("tasksToCancel", tasksToCancel.size());
    // cancel the tasks
    for (auto const& it : tasksToCancel)
    {
        PSI_FRAMEWORK_LOG(INFO)
            << LOG_DESC("handleTaskSyncInfo: cancel task that no longer exist in-peer")
            << LOG_KV("task", it);
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::TaskNotFound,
            "Cancel task " + it + " for task no longer exists in the peer-party!");
        cancelTask(std::move(error), it);
    }
}

void PSIFramework::handleTaskNotificationMsg(PSIMessageInterface::Ptr _msg)
{
    auto notificationResult = std::dynamic_pointer_cast<PSITaskNotificationMessage>(_msg);
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("handleTaskNotificationMsg") << printPSIMessage(_msg)
                            << LOG_KV("code", notificationResult->errorCode())
                            << LOG_KV("msg", notificationResult->errorMessage());
    // cancel the local task
    auto error = std::make_shared<bcos::Error>(
        notificationResult->errorCode(), notificationResult->errorMessage());
    cancelTask(std::move(error), notificationResult->taskID());
}

bool PSIFramework::handlePSIFrameworkMsg(PSIMessageInterface::Ptr _msg)
{
    // Note: exception should been catched by the caller
    switch (_msg->packetType())
    {
    case (uint32_t)PSIPacketType::CancelTaskNotification:
    {
        handleTaskNotificationMsg(_msg);
        break;
    }
    case (uint32_t)PSIPacketType::TaskSyncMsg:
    {
        handleTaskSyncInfo(_msg);
        break;
    }
    case (uint32_t)PSIPacketType::HandshakeResponse:
    {
        onHandshakeResponse(_msg);
        break;
    }
    case (uint32_t)PSIPacketType::HandshakeRequest:
    {
        onHandshakeRequest(_msg);
        break;
    }
    case (uint32_t)PSIPacketType::PSIResultSyncMsg:
    {
        handlePSIResultSyncMsg(_msg);
        break;
    }
    default:
    {
        return false;
    }
    }
    return true;
}

LineReader::Ptr PSIFramework::loadData(DataResourceLoader::Ptr _dataResourceLoader,
    std::string const& _taskID, DataResource::ConstPtr const& _dataResource)
{
    // load data from the data-source(TODO: loadReader use async-interface)
    auto reader =
        _dataResourceLoader->loadReader(_dataResource->desc(), ppc::io::DataSchema::Bytes, true);
    // only support one-column
    if (reader->columnSize() == 0 || reader->columnSize() > 1)
    {
        auto errorMsg = "load data for task " + _taskID +
                        "failed, PSI dataSourcemust be on column, current column size is " +
                        std::to_string(reader->columnSize());
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::LoadDataFailed, errorMsg));
    }
    return reader;
}

void PSIFramework::sendHandshakeRequest(TaskState::Ptr const& _taskState)
{
    if (_taskState->peerID().empty())
    {
        return;
    }
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("sendHandshakeRequest")
                            << printTaskInfo(_taskState->task());
    auto handshakeReq =
        m_msgFactory->createHandshakeRequest((uint32_t)PSIPacketType::HandshakeRequest);
    handshakeReq->setCurves(
        g_PPCConfig.supportedCurves((uint8_t)TaskType::PSI, (uint8_t)m_psiConfig->algorithmType()));
    handshakeReq->setHashList(g_PPCConfig.supportedHashList(
        (uint8_t)TaskType::PSI, (uint8_t)m_psiConfig->algorithmType()));
    handshakeReq->setTaskID(_taskState->task()->id());
    handshakeReq->setFrom(m_psiConfig->selfParty());
    m_psiConfig->generateAndSendPPCMessage(_taskState->peerID(), _taskState->task()->id(),
        handshakeReq, [this, _taskState, handshakeReq](bcos::Error::Ptr&& _error) {
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            PSI_FRAMEWORK_LOG(WARNING)
                << LOG_DESC("sendHandshakeRequest error") << LOG_KV("code", _error->errorCode())
                << LOG_KV("msg", _error->errorMessage());
            auto resource = _taskState->task()->selfParty()->dataResource();
            onTaskError("sendHandshakeRequest", std::move(_error), _taskState->peerID(),
                _taskState->task()->id(), resource ? resource->resourceID() : "emptyResource");
        });
}


void PSIFramework::responsePSIResultSyncStatus(int32_t _code, std::string const& _msg,
    bcos::bytes const& _peer, std::string const& _taskID, std::string const& _uuid, uint32_t _seq)
{
    // response to the client
    auto psiMsg =
        m_msgFactory->createTaskNotificationMessage((uint32_t)PSIPacketType::PSIResultSyncResponse);
    psiMsg->setErrorCode(_code);
    psiMsg->setErrorMessage(_msg);
    m_psiConfig->asyncSendResponse(
        _peer, _taskID, _uuid, psiMsg,
        [this](bcos::Error::Ptr _error) {
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            PSI_FRAMEWORK_LOG(WARNING)
                << LOG_DESC("responsePSIResultSyncStatus error")
                << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
        },
        _seq);
}

void PSIFramework::handlePSIResultSyncMsg(PSIMessageInterface::Ptr _resultSyncMsg)
{
    PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("handlePSIResultSyncMsg")
                            << printPSIMessage(_resultSyncMsg);
    auto taskState = getTaskByID(_resultSyncMsg->taskID());
    if (!taskState)
    {
        PSI_FRAMEWORK_LOG(WARNING)
            << LOG_DESC("handlePSIResultSyncMsg error for the task not found")
            << printPSIMessage(_resultSyncMsg);
        std::string msg =
            "sync psi result for task " + _resultSyncMsg->taskID() + " failed for task not found!";
        responsePSIResultSyncStatus((int32_t)PSIRetCode::TaskNotFound, msg,
            _resultSyncMsg->fromNode(), _resultSyncMsg->taskID(), _resultSyncMsg->uuid(),
            _resultSyncMsg->seq());
        return;
    }
    try
    {
        taskState->storePSIResult(m_dataResourceLoader, _resultSyncMsg->takeData());
        responsePSIResultSyncStatus((int32_t)PSIRetCode::Success, "success",
            _resultSyncMsg->fromNode(), _resultSyncMsg->taskID(), _resultSyncMsg->uuid(),
            _resultSyncMsg->seq());
    }
    catch (std::exception const& e)
    {
        PSI_FRAMEWORK_LOG(WARNING) << LOG_DESC("handlePSIResultSyncMsg exception")
                                   << LOG_KV("error", boost::diagnostic_information(e));
        auto errorMessage = "sync psi result for " + _resultSyncMsg->taskID() +
                            " failed, error: " + std::string(boost::diagnostic_information(e));
        responsePSIResultSyncStatus((int32_t)PSIRetCode::SyncPSIResultFailed, errorMessage,
            _resultSyncMsg->fromNode(), _resultSyncMsg->taskID(), _resultSyncMsg->uuid(),
            _resultSyncMsg->seq());
        // cancel the task
        auto error = BCOS_ERROR_PTR((int32_t)PSIRetCode::SyncPSIResultFailed, errorMessage);
        cancelTask(std::move(error), _resultSyncMsg->taskID());
    }
}