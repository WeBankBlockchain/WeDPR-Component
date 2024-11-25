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
 * @file LabeledPSIImpl.cpp
 * @author: shawnhe
 * @date 2022-11-7
 */

#include "LabeledPSIImpl.h"
#include "Common.h"
#include "LabeledPSI.h"
#include "core/LabeledPSIParams.h"
#include "core/SenderDB.h"
#include "core/TaskCommand.h"
#include "ppc-psi/src/labeled-psi/core/LabeledPSIReceiver.h"
#include "wedpr-protocol/tars/TarsSerialize.h"

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::tools;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace ppc::task;

LabeledPSIImpl::LabeledPSIImpl(LabeledPSIConfig::Ptr _config, unsigned _idleTimeMs)
  : Worker("LabeledPSI", _idleTimeMs),
    TaskGuarder(_config, TaskAlgorithmType::LABELED_PSI_2PC, "Labeled-PSI-Timer"),
    m_config(std::move(_config)),
    m_msgQueue(std::make_shared<LabeledPSIMsgQueue>()),
    m_worker(std::make_shared<bcos::ThreadPool>("senderDB-worker", 1))
{
    m_sender = make_shared<LabeledPSISender>(
        m_config, [&](const std::string& _taskID, bcos::Error::Ptr _error) {
            if (_error && _error->errorCode())
            {
                onSelfError(_taskID, std::move(_error), true);
            }
            else
            {
                onSenderTaskDone(_taskID);
            }
        });
}

// run task
void LabeledPSIImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, TaskResponseCallback&& _onTaskFinished)
{
    auto onTaskFinished = [self = weak_from_this(), taskID = _task->id(), _onTaskFinished](
                              ppc::protocol::TaskResult::Ptr&& _result) {
        try
        {
            auto result = std::move(_result);
            _onTaskFinished(std::move(result));
            LABELED_PSI_LOG(INFO) << LOG_DESC("finish a task") << LOG_KV("taskID", taskID);
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }

            // erase the taskInfo from the gateway
            psi->m_config->front()->eraseTaskInfo(taskID);
        }
        catch (std::exception& e)
        {
            LABELED_PSI_LOG(ERROR) << LOG_DESC("handle callback error after finishing task")
                                   << LOG_KV("exception", boost::diagnostic_information(e));
        }
    };

    try
    {
        // there is always a self-help party
        auto role = _task->selfParty()->partyIndex();
        if (role == uint16_t(PartyType::Client))
        {
            auto error = checkTask(_task, 2, true, true, false);
            if (error)
            {
                LABELED_PSI_LOG(WARNING)
                    << LOG_DESC("failed to check task, " + error->errorMessage())
                    << printTaskInfo(_task);
                // notice peer to cancel task
                noticePeerToFinish(_task);

                auto result = std::make_shared<TaskResult>(_task->id());
                result->setError(std::move(error));
                onTaskFinished(std::move(result));
                return;
            }

            // add pending task
            auto taskState = m_taskStateFactory->createTaskState(_task, std::move(onTaskFinished));
            taskState->setPeerID(getPeerID(_task));
            taskState->registerNotifyPeerFinishHandler([self = weak_from_this(), _task]() {
                auto psi = self.lock();
                if (!psi)
                {
                    return;
                }

                psi->noticePeerToFinish(_task);
            });
            addPendingTask(taskState);
            // check the memory
            checkHostResource(m_config->minNeededMemoryGB());

            auto oprfClient = std::make_shared<EcdhOprfClient>(
                sizeof(apsi::Item::value_type) + sizeof(apsi::LabelKey), m_config->hash(),
                m_config->eccCrypto());
            auto receiver = std::make_shared<LabeledPSIReceiver>(m_config, taskState, oprfClient);
            receiver->asyncRunTask();
            addReceiver(receiver);

            // notify the taskInfo to the front
            error = m_config->front()->notifyTaskInfo(_task->id());
            if (error && error->errorCode())
            {
                onSelfError(_task->id(), error, true);
            }
        }
        else if (role == uint16_t(PartyType::Server))
        {
            asyncRunSenderTask(_task, std::move(onTaskFinished));
        }
        else
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("undefined task role") << unsigned(role);
            // notice peer to cancel task
            noticePeerToFinish(_task);

            auto result = std::make_shared<TaskResult>(_task->id());
            result->setError(std::make_shared<bcos::Error>(
                (int)LabeledPSIRetCode::UNDEFINED_TASK_ROLE, "undefined task role"));
            onTaskFinished(std::move(result));
            noticePeerToFinish(_task);
            return;
        }
    }
    catch (std::exception const& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("asyncRunTask exception")
                                 << LOG_KV("task", printTaskInfo(_task))
                                 << LOG_KV("error", boost::diagnostic_information(e));
        auto error = std::make_shared<bcos::Error>(
            -1, "asyncRunTask failed for " + boost::diagnostic_information(e));
        onSelfError(_task->id(), error, true);
    }
}

// register to the front to get the message related to labeled-psi
void LabeledPSIImpl::onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg)
{
    try
    {
        m_msgQueue->push(_msg);

        // notify to handle the message
        wakeupWorker();
    }
    catch (std::exception const& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("onReceiveMessage exception") << printPPCMsg(_msg)
                                 << LOG_KV("error", boost::diagnostic_information(e));
    }
}

void LabeledPSIImpl::start()
{
    if (m_started)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("The LabeledPSI has already been started");
        return;
    }
    LABELED_PSI_LOG(INFO) << LOG_DESC("Start the LabeledPSI");
    m_started = true;

    // start a thread to execute task
    startWorking();

    startPingTimer();
}

void LabeledPSIImpl::stop()
{
    if (!m_started)
    {
        return;
    }
    LABELED_PSI_LOG(INFO) << LOG_DESC("Stop LabeledPSI");
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
    stopPingTimer();

    LABELED_PSI_LOG(INFO) << LOG_DESC("LabeledPSI stopped");
}

void LabeledPSIImpl::onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message)
{
    LABELED_PSI_LOG(WARNING) << LOG_DESC("onReceivedErrorNotification") << printPPCMsg(_message);
    // finish the task while the peer is failed
    auto taskState = findPendingTask(_message->taskID());
    if (taskState)
    {
        taskState->onPeerNotifyFinish();
    }
}

void LabeledPSIImpl::onSelfError(
    const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer)
{
    LABELED_PSI_LOG(WARNING) << LOG_DESC("onSelfError") << LOG_KV("task", _taskID)
                             << LOG_KV("error", _error->errorMessage())
                             << LOG_KV("noticePeer", _noticePeer);
    try
    {
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
    catch (std::exception& e)
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("onSelfError")
                               << LOG_KV("exception", boost::diagnostic_information(e));
    }
}

void LabeledPSIImpl::asyncRunSenderTask(
    const ppc::protocol::Task::ConstPtr& _task, TaskResponseCallback&& _onTaskFinished)
{
    TaskCommand::Ptr taskCommand;
    try
    {
        // parse the task command
        taskCommand = std::make_shared<TaskCommand>(_task->param());
    }
    catch (const std::exception& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("asyncRunSenderTask exception") << printTaskInfo(_task)
                                 << LOG_KV("error", boost::diagnostic_information(e));

        auto result = std::make_shared<protocol::TaskResult>(_task->id());
        result->setError(std::make_shared<bcos::Error>((int)LabeledPSIRetCode::ON_EXCEPTION,
            "exception caught while asyncRunSenderTask: " + boost::diagnostic_information(e)));
        _onTaskFinished(std::move(result));
        noticePeerToFinish(_task);
        return;
    }

    switch (taskCommand->command())
    {
    case (int)LabeledPSICommand::SETUP_SENDER_DB:
    {
        if (m_senderReady)
        {
            LABELED_PSI_LOG(WARNING)
                << LOG_DESC("sender has been set up: ") << printTaskInfo(_task);
            auto result = std::make_shared<protocol::TaskResult>(_task->id());
            result->setError(std::make_shared<bcos::Error>(
                (int)LabeledPSIRetCode::SETUP_SENDER_ERROR, "sender has been set up"));
            _onTaskFinished(std::move(result));
            return;
        }

        auto error = checkTask(_task, 1, true, false, false);
        if (error)
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("failed to check task, " + error->errorMessage())
                                     << printTaskInfo(_task);
            auto result = std::make_shared<TaskResult>(_task->id());
            result->setError(std::move(error));
            _onTaskFinished(std::move(result));
            return;
        }

        auto taskState =
            m_taskStateFactory->createTaskState(_task, std::move(_onTaskFinished), true);
        addPendingTask(taskState);

        m_worker->enqueue([self = weak_from_this(), _task, taskCommand]() {
            auto psi = self.lock();
            if (psi)
            {
                auto labelByteCount = taskCommand->args()[0];
                psi->setupSenderDB(_task, labelByteCount);
            }
        });
        break;
    }
    case (int)LabeledPSICommand::RUN_LABELED_PSI:
    {
        if (!m_senderReady)
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("sender not ready: ") << printTaskInfo(_task);

            // notice peer to cancel task
            noticePeerToFinish(_task);

            auto result = std::make_shared<protocol::TaskResult>(_task->id());
            result->setError(std::make_shared<bcos::Error>(
                (int)LabeledPSIRetCode::SENDER_NOT_READY, "sender not ready"));
            _onTaskFinished(std::move(result));
            return;
        }

        auto error = checkTask(_task, 2, false, false, false, false);
        if (error)
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("failed to check task, " + error->errorMessage())
                                     << printTaskInfo(_task);
            // notice peer to cancel task
            noticePeerToFinish(_task);
            auto result = std::make_shared<TaskResult>(_task->id());
            result->setError(std::move(error));
            _onTaskFinished(std::move(result));
            return;
        }

        auto taskState = m_taskStateFactory->createTaskState(_task, std::move(_onTaskFinished));
        taskState->setPeerID(getPeerID(_task));
        taskState->registerNotifyPeerFinishHandler([self = weak_from_this(), _task]() {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }

            psi->noticePeerToFinish(_task);
        });
        addPendingTask(taskState);

        // notify the taskInfo to the front
        error = m_config->front()->notifyTaskInfo(_task->id());
        if (error && error->errorCode())
        {
            onSelfError(_task->id(), error, true);
        }
        break;
    }
    case (int)LabeledPSICommand::SAVE_SENDER_CACHE:
    {
        if (!m_senderReady)
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("sender not ready: ") << printTaskInfo(_task);
            auto result = std::make_shared<protocol::TaskResult>(_task->id());
            result->setError(std::make_shared<bcos::Error>(
                (int)LabeledPSIRetCode::SENDER_NOT_READY, "sender not ready"));
            _onTaskFinished(std::move(result));
            return;
        }

        auto error = checkTask(_task, 1, false, true, false);
        if (error)
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("failed to check task, " + error->errorMessage())
                                     << printTaskInfo(_task);
            auto result = std::make_shared<TaskResult>(_task->id());
            result->setError(std::move(error));
            _onTaskFinished(std::move(result));
            noticePeerToFinish(_task);
            return;
        }

        auto taskState =
            m_taskStateFactory->createTaskState(_task, std::move(_onTaskFinished), true);
        addPendingTask(taskState);

        m_worker->enqueue([self = weak_from_this(), _task]() {
            auto psi = self.lock();
            if (psi)
            {
                psi->saveSenderCache(_task);
            }
        });
        break;
    }
    case (int)LabeledPSICommand::LOAD_SENDER_CACHE:
    {
        if (m_senderReady)
        {
            LABELED_PSI_LOG(WARNING)
                << LOG_DESC("sender has been set up: ") << printTaskInfo(_task);
            auto result = std::make_shared<protocol::TaskResult>(_task->id());
            result->setError(std::make_shared<bcos::Error>(
                (int)LabeledPSIRetCode::LOAD_SENDER_CACHE_ERROR, "sender has been set up"));
            _onTaskFinished(std::move(result));
            return;
        }

        auto error = checkTask(_task, 1, true, false, false);
        if (error)
        {
            LABELED_PSI_LOG(WARNING) << LOG_DESC("failed to check task, " + error->errorMessage())
                                     << printTaskInfo(_task);
            auto result = std::make_shared<TaskResult>(_task->id());
            result->setError(std::move(error));
            _onTaskFinished(std::move(result));
            noticePeerToFinish(_task);
            return;
        }

        auto taskState =
            m_taskStateFactory->createTaskState(_task, std::move(_onTaskFinished), true);
        addPendingTask(taskState);
        m_worker->enqueue([self = weak_from_this(), _task]() {
            auto psi = self.lock();
            if (psi)
            {
                psi->loadSenderCache(_task);
            }
        });
        break;
    }
    default:
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("unsupported command: ") << printTaskInfo(_task);
        auto result = std::make_shared<protocol::TaskResult>(_task->id());
        result->setError(std::make_shared<bcos::Error>(
            (int)LabeledPSIRetCode::UNDEFINED_COMMAND, "unsupported command"));
        _onTaskFinished(std::move(result));
        noticePeerToFinish(_task);
        break;
    }
    }
}

void LabeledPSIImpl::setupSenderDB(const ppc::protocol::Task::ConstPtr& _task, int _labelByteCount)
{
    try
    {
        auto dataResource = _task->selfParty()->dataResource();
        auto reader = loadReader(_task->id(), dataResource, DataSchema::Bytes, 2);

        DataBatch::Ptr items, labels;
        if (reader->type() == ppc::protocol::DataResourceType::MySQL)
        {
            items = reader->next(0, DataSchema::Bytes);
            labels = reader->next(1, DataSchema::Bytes);
        }
        else
        {
            auto lines = reader->next(-1, DataSchema::Bytes);
            items = std::make_shared<DataBatch>();
            items->setDataSchema(ppc::io::DataSchema::Bytes);
            labels = std::make_shared<DataBatch>();
            labels->setDataSchema(ppc::io::DataSchema::Bytes);
            for (uint32_t i = 0; i < lines->size(); ++i)
            {
                auto line = lines->getBytes(i);
                std::vector<std::string> data;
                boost::split(data, line, boost::is_any_of(","));
                if (data.size() != 2)
                {
                    BOOST_THROW_EXCEPTION(BCOS_ERROR((int)LabeledPSIRetCode::DATA_FORMAT_ERROR,
                        "the data format should be: key,value"));
                }

                items->append(bcos::bytes(data[0].begin(), data[0].end()));
                labels->append(bcos::bytes(data[1].begin(), data[1].end()));
            }
        }

        if (items->size() != labels->size())
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)LabeledPSIRetCode::DATA_FORMAT_ERROR,
                "the number of items and labels is not the same"));
        }

        LABELED_PSI_LOG(INFO) << LOG_DESC("finish loading item-label")
                              << LOG_KV("count", items->size());

        auto psiParams = getPsiParams(items->size());

        m_senderDB = std::make_shared<SenderDB>(
            psiParams, m_config->oprfServer(), _labelByteCount, 16, false);

        std::set<bcos::bytes> tmp;
        for (uint32_t i = 0; i < items->size(); i++)
        {
            tmp.insert(items->getBytes(i));
        }
        if (tmp.size() < items->size())
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int)LabeledPSIRetCode::DATA_FORMAT_ERROR, "duplicate key found"));
        }

        m_senderDB->setData(items, labels);

        m_sender->setSenderDB(m_senderDB);
        m_senderReady.exchange(true);

        LABELED_PSI_LOG(INFO) << LOG_DESC("setupSenderDB done") << printTaskInfo(_task);

        onSenderTaskDone(_task->id());
    }
    catch (bcos::Error const& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("setupSenderDB exception") << printTaskInfo(_task)
                                 << LOG_KV("code", e.errorCode())
                                 << LOG_KV("msg", e.errorMessage());
        onSelfError(
            _task->id(), std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), false);
    }
    catch (const std::exception& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("setupSenderDB exception") << printTaskInfo(_task)
                                 << LOG_KV("error", boost::diagnostic_information(e));

        onSelfError(_task->id(),
            std::make_shared<bcos::Error>((int)LabeledPSIRetCode::ON_EXCEPTION,
                "exception caught while setupSenderDB: " + boost::diagnostic_information(e)),
            false);
    }
}

void LabeledPSIImpl::saveSenderCache(const ppc::protocol::Task::ConstPtr& _task)
{
    try
    {
        auto dataResource = _task->selfParty()->dataResource();

        LineWriter::Ptr writer;
        if (!_task->enableOutputExists())
        {
            // Note: if the output-resource already exists, will throw exception
            m_config->dataResourceLoader()->checkResourceExists(dataResource->outputDesc());
        }
        writer = m_config->dataResourceLoader()->loadWriter(dataResource->outputDesc(), true);

        bcos::bytes out;
        m_senderDB->saveToBytes(out);

        writer->writeBytes(bcos::ref(out));
        writer->flush();
        writer->close();

        onSenderTaskDone(_task->id());
        LABELED_PSI_LOG(INFO) << LOG_DESC("saveSenderCache done") << printTaskInfo(_task);
    }
    catch (bcos::Error const& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("saveSenderCache exception") << printTaskInfo(_task)
                                 << LOG_KV("code", e.errorCode())
                                 << LOG_KV("msg", e.errorMessage());
        onSelfError(
            _task->id(), std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), false);
    }
    catch (const std::exception& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("saveSenderCache exception") << printTaskInfo(_task)
                                 << LOG_KV("error", boost::diagnostic_information(e));

        onSelfError(_task->id(),
            std::make_shared<bcos::Error>((int)LabeledPSIRetCode::ON_EXCEPTION,
                "exception caught while saveSenderCache: " + boost::diagnostic_information(e)),
            false);
    }
}

void LabeledPSIImpl::loadSenderCache(const ppc::protocol::Task::ConstPtr& _task)
{
    try
    {
        auto dataResource = _task->selfParty()->dataResource();
        auto reader = m_config->dataResourceLoader()->loadReader(
            dataResource->desc(), ppc::io::DataSchema::Bytes, false);

        auto cache = reader->readBytes();
        m_senderDB = SenderDB::loadFromBytes(m_config->oprfServer(), cache);

        m_senderReady.exchange(true);
        m_sender->setSenderDB(m_senderDB);
        onSenderTaskDone(_task->id());
        LABELED_PSI_LOG(INFO) << LOG_DESC("loadSenderCache done") << printTaskInfo(_task);
    }
    catch (bcos::Error const& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("loadSenderCache exception") << printTaskInfo(_task)
                                 << LOG_KV("code", e.errorCode())
                                 << LOG_KV("msg", e.errorMessage());
        onSelfError(
            _task->id(), std::make_shared<bcos::Error>(e.errorCode(), e.errorMessage()), false);
    }
    catch (const std::exception& e)
    {
        LABELED_PSI_LOG(WARNING) << LOG_DESC("loadSenderCache exception") << printTaskInfo(_task)
                                 << LOG_KV("error", boost::diagnostic_information(e));

        onSelfError(_task->id(),
            std::make_shared<bcos::Error>((int)LabeledPSIRetCode::ON_EXCEPTION,
                "exception caught while loadSenderCache: " + boost::diagnostic_information(e)),
            false);
    }
}

// labeled-psi main processing function
// for ut to make this function public
void LabeledPSIImpl::executeWorker()
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

void LabeledPSIImpl::checkFinishedTask()
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
        removePendingTask(taskID);
    }
}

void LabeledPSIImpl::onSenderTaskDone(const std::string& _taskID)
{
    auto taskState = findPendingTask(_taskID);
    if (taskState)
    {
        taskState->onTaskFinished(std::make_shared<protocol::TaskResult>(_taskID), false);
    }
}

void LabeledPSIImpl::handleReceivedMessage(const ppc::front::PPCMessageFace::Ptr& _message)
{
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
            case int(LabeledPSIMessageType::PARAMS_REQUEST):
            {
                psi->onReceivePsiParamsRequest(_message);
                break;
            }
            case int(LabeledPSIMessageType::PARAMS_RESPONSE):
            {
                psi->onReceivePsiParamsResponse(_message);
                break;
            }
            case int(LabeledPSIMessageType::OPRF_BLINDED_ITEMS):
            {
                psi->onReceiveBlindedItems(_message);
                break;
            }
            case int(LabeledPSIMessageType::OPRF_EVALUATED_ITEMS):
            {
                psi->onReceiveEvaluatedItems(_message);
                break;
            }
            case int(LabeledPSIMessageType::QUERY):
            {
                psi->onReceiveQuery(_message);
                break;
            }
            case int(LabeledPSIMessageType::RESPONSE):
            {
                psi->onReceiveResponse(_message);
                break;
            }
            default:
            {
                LABELED_PSI_LOG(WARNING)
                    << LOG_DESC("unsupported messageType ") << unsigned(_message->messageType());
                break;
            }
            }
        }
        catch (std::exception const& e)
        {
            LABELED_PSI_LOG(WARNING)
                << LOG_DESC("handleReceivedMessage exception")
                << LOG_KV("type", unsigned(_message->messageType())) << printPPCMsg(_message)
                << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}


void LabeledPSIImpl::onReceivePsiParamsRequest(ppc::front::PPCMessageFace::Ptr _message)
{
    if (!m_senderReady)
    {
        noticePeerToFinish(_message->taskID(), _message->sender());
    }
    else
    {
        auto taskID = _message->taskID();
        if (findPendingTask(taskID))
        {
            m_sender->handlePsiParamsRequest(std::move(_message));
        }
    }
}

void LabeledPSIImpl::onReceivePsiParamsResponse(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->handlePsiParams(std::move(_message));
    }
}

void LabeledPSIImpl::onReceiveBlindedItems(ppc::front::PPCMessageFace::Ptr _message)
{
    auto taskID = _message->taskID();
    if (findPendingTask(taskID))
    {
        m_sender->handleBlindedItems(std::move(_message));
    }
}

void LabeledPSIImpl::onReceiveEvaluatedItems(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->handleEvaluatedItems(std::move(_message));
    }
}

void LabeledPSIImpl::onReceiveQuery(ppc::front::PPCMessageFace::Ptr _message)
{
    auto taskID = _message->taskID();
    if (findPendingTask(taskID))
    {
        m_sender->handleQuery(std::move(_message));
    }
}


void LabeledPSIImpl::onReceiveResponse(ppc::front::PPCMessageFace::Ptr _message)
{
    auto receiver = findReceiver(_message->taskID());
    if (receiver)
    {
        receiver->handleOneResponse(std::move(_message));
    }
}
