/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file BsEcdhPSIImpl.cpp
 * @author: shawnhe
 * @date 2023-09-20
 */

#include "BsEcdhPSIImpl.h"

using namespace bcos;
using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;

BsEcdhResult::Ptr BsEcdhPSIImpl::getTaskStatus(GetTaskStatusRequest::Ptr _request)
{
    ReadGuard l(x_tasks);
    auto state = m_taskStates.find(_request->taskID);
    if (state == m_taskStates.end())
    {
        auto result = std::make_shared<BsEcdhResult>(_request->taskID);
        result->setError(std::make_shared<Error>(TaskNotFound, "Task not found."));
        return result;
    }

    if (isNotExecutable(state->second->status()))
    {
        return state->second->result();
    }

    return prepareResultWithoutLock(_request->taskID, toString(state->second->status()));
}

BsEcdhResult::Ptr BsEcdhPSIImpl::updateTaskStatus(UpdateTaskStatusRequest::Ptr _request)
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("update task status") << LOG_KV("taskID", _request->taskID)
                          << LOG_KV("status", _request->status);

    WriteGuard l(x_tasks);
    auto state = m_taskStates.find(_request->taskID);
    if (state == m_taskStates.end())
    {
        auto result = std::make_shared<BsEcdhResult>(_request->taskID);
        result->setError(std::make_shared<Error>(TaskNotFound, "Task not found."));
        return result;
    }

    auto status = fromString(_request->status);
    if (status == TaskStatus::RUNNING)
    {
        state->second->restartTask();
    }
    else if (status == TaskStatus::PAUSING)
    {
        state->second->pauseTask();
    }

    if (isNotExecutable(state->second->status()))
    {
        return state->second->result();
    }

    return prepareResultWithoutLock(_request->taskID, toString(state->second->status()));
}


BsEcdhResult::Ptr BsEcdhPSIImpl::killTask(KillTaskRequest::Ptr _request)
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("kill task") << LOG_KV("taskID", _request->taskID);

    WriteGuard l(x_tasks);
    auto result = std::make_shared<BsEcdhResult>(_request->taskID);
    auto state = m_taskStates.find(_request->taskID);
    if (state == m_taskStates.end())
    {
        result->setError(std::make_shared<Error>(TaskNotFound, "Task not found."));
        return result;
    }

    if (isExecutable(state->second->status()))
    {
        GetTaskStatusResponse response;
        response.taskID = _request->taskID;
        response.status = toString(TaskStatus::FAILED);
        auto ecdhResult = std::make_shared<BsEcdhResult>(_request->taskID, response.serialize());
        ecdhResult->setError(std::make_shared<Error>(TaskKilled, "Task has been killed."));
        state->second->setResult(ecdhResult);
        state->second->updateStatus(TaskStatus::FAILED);
    }

    auto cache = m_taskCaches.find(state->second->taskID());
    if (cache != m_taskCaches.end())
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("clean finished task cache")
                              << LOG_KV("taskID", cache->second->taskID());
        m_taskCaches.erase(cache);
    }

    return std::make_shared<BsEcdhResult>(_request->taskID);
}

void BsEcdhPSIImpl::asyncRunTask(
    RunTaskRequest::Ptr _request, std::function<void(BsEcdhResult::Ptr&&)>&& _onTaskFinished)
{
    try
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("receive a task")
                              << LOG_KV("request", _request->toString());
        auto state = findTaskState(_request->taskID);
        if (state)
        {
            BS_ECDH_PSI_LOG(WARNING)
                << LOG_DESC("asyncRunTask, task exists.") << LOG_KV("taskID", _request->taskID);
            if (_onTaskFinished)
            {
                _onTaskFinished(std::make_shared<BsEcdhResult>(_request->taskID));
            }
            return;
        }

        if (taskCount() >= MAX_TASK_COUNT)
        {
            BS_ECDH_PSI_LOG(WARNING)
                << LOG_DESC("task count reach max.") << LOG_KV("taskID", _request->taskID);
            auto result = std::make_shared<BsEcdhResult>(_request->taskID);
            result->setError(std::make_shared<Error>(TaskCountReachMax, "Task count reaches max."));
            if (_onTaskFinished)
            {
                _onTaskFinished(std::move(result));
            }
            return;
        }

        // init task state
        auto taskState = std::make_shared<BsEcdhTaskState>(
            _request->taskID, TaskStatus::PENDING, m_timeoutMinutes);
        // check the memory
        checkHostResource(m_config->minNeededMemoryGB());
        // create ecdh cache
        auto cache = std::make_shared<BsEcdhCache>(
            _request->taskID, m_threadPool, m_taskGuarder, _request->dataResource,
            _request->enableAudit, m_enableOutputExists,
            [self = weak_from_this(), taskID = _request->taskID] {
                auto psi = self.lock();
                if (psi)
                {
                    psi->onSelfCiphersReady(taskID);
                }
            },
            [self = weak_from_this(), taskID = _request->taskID] {
                auto psi = self.lock();
                if (psi)
                {
                    psi->onAllCiphersReady(taskID);
                }
            },
            [self = weak_from_this(), taskID = _request->taskID](
                protocol::TaskStatus _status, BsEcdhResult::Ptr _result) {
                auto psi = self.lock();
                if (psi)
                {
                    psi->onTaskFinished(taskID, _status, std::move(_result));
                }
            },
            _request->partnerInputsSize);
        cache->start();

        // add task
        addTask(std::move(taskState), std::move(cache));

        if (_onTaskFinished)
        {
            _onTaskFinished(std::make_shared<BsEcdhResult>(_request->taskID));
        }
    }
    catch (std::exception const& e)
    {
        BS_ECDH_PSI_LOG(ERROR) << LOG_DESC("asyncRunTask")
                               << LOG_KV("exception", boost::diagnostic_information(e));
        auto result = std::make_shared<BsEcdhResult>(_request->taskID);
        result->setError(std::make_shared<Error>(
            TaskParamsError, "Init task error: " + boost::diagnostic_information(e)));
        if (_onTaskFinished)
        {
            _onTaskFinished(std::move(result));
        }
    }
}

BsEcdhResult::Ptr BsEcdhPSIImpl::fetchCipher(FetchCipherRequest::Ptr _request)
{
    auto result = checkTaskRequest(_request->taskID);
    if (result.first)
    {
        return result.first;
    }
    return result.second->fetchCipher(std::move(_request));
}

BsEcdhResult::Ptr BsEcdhPSIImpl::sendEcdhCipher(SendEcdhCipherRequest::Ptr _request)
{
    auto result = checkTaskRequest(_request->taskID);
    if (result.first)
    {
        return result.first;
    }
    return result.second->onEcdhCipherReceived(std::move(_request));
}

BsEcdhResult::Ptr BsEcdhPSIImpl::sendPartnerCipher(SendPartnerCipherRequest::Ptr _request)
{
    auto result = checkTaskRequest(_request->taskID);
    if (result.first)
    {
        return result.first;
    }
    return result.second->onPartnerCipherReceived(std::move(_request));
}

std::pair<BsEcdhResult::Ptr, BsEcdhCache::Ptr> BsEcdhPSIImpl::checkTaskRequest(
    const std::string& _taskID)
{
    auto result = std::make_shared<BsEcdhResult>(_taskID);

    auto state = findTaskState(_taskID);
    if (!state)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("checkTaskRequest, task not found.")
                              << LOG_KV("taskID", _taskID);
        result->setError(std::make_shared<Error>(TaskNotFound, "Task not found."));
        return {result, nullptr};
    }

    // Even if the task is completed, the browser can continue to send ciphers.
    if (state->status() != TaskStatus::RUNNING && state->status() != TaskStatus::PAUSING &&
        state->status() != TaskStatus::COMPLETED)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("checkTaskRequest, task is not running.")
                              << LOG_KV("taskID", _taskID);
        result->setError(std::make_shared<Error>(TaskIsNotRunning, "Task is not running."));
        return {result, nullptr};
    }

    auto cache = findTaskCache(_taskID);
    if (!cache)
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("checkTaskRequest, task not found.")
                              << LOG_KV("taskID", _taskID);
        result->setError(std::make_shared<Error>(TaskNotFound, "Task cache not found."));
        return {result, nullptr};
    }

    state->active();
    return {nullptr, cache};
}

void BsEcdhPSIImpl::start()
{
    if (m_started)
    {
        BS_ECDH_PSI_LOG(ERROR) << LOG_DESC("The BS-ECDH-PSI has already been started");
        return;
    }
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("Start the BS-ECDH-PSI");
    m_started = true;

    if (m_taskCleaner)
    {
        m_taskCleaner->registerTimeoutHandler([self = weak_from_this()] {
            auto psi = self.lock();
            if (psi)
            {
                psi->checkAndCleanTask();
            }
        });
        m_taskCleaner->start();
    }
}


void BsEcdhPSIImpl::stop()
{
    if (!m_started)
    {
        return;
    }
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("Stop BS-ECDH-PSI");
    m_started = false;

    if (m_taskCleaner)
    {
        m_taskCleaner->stop();
    }
}

void BsEcdhPSIImpl::checkAndCleanTask()
{
    try
    {
        WriteGuard l(x_tasks);
        for (auto state = m_taskStates.begin(); state != m_taskStates.end();)
        {
            state->second->autoPauseChecking();
            if (isNotExecutable(state->second->status()) || state->second->isTimeout() ||
                state->second->isExpired())
            {
                auto cache = m_taskCaches.find(state->second->taskID());
                if (cache != m_taskCaches.end())
                {
                    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("clean finished task cache")
                                          << LOG_KV("taskID", cache->second->taskID());
                    m_taskCaches.erase(cache);
                }
            }

            if (state->second->isExpired())
            {
                BS_ECDH_PSI_LOG(INFO)
                    << LOG_DESC("clean expired task") << LOG_KV("taskID", state->second->taskID());
                state = m_taskStates.erase(state);
            }
            else
            {
                ++state;
            }
        }
    }
    catch (std::exception const& e)
    {
        BS_ECDH_PSI_LOG(ERROR) << LOG_DESC("checkAndCleanTask")
                               << LOG_KV("exception", boost::diagnostic_information(e));
    }
    m_taskCleaner->restart();
}

void BsEcdhPSIImpl::onSelfCiphersReady(const std::string& _taskID)
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onSelfCiphersReady") << LOG_KV("taskID", _taskID);
    WriteGuard l(x_tasks);
    auto state = m_taskStates.find(_taskID);
    if (state != m_taskStates.end())
    {
        state->second->setupAutoPause();
    }
}

void BsEcdhPSIImpl::onAllCiphersReady(const std::string& _taskID)
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("allCiphersReady") << LOG_KV("taskID", _taskID);
    WriteGuard l(x_tasks);
    auto state = m_taskStates.find(_taskID);
    if (state != m_taskStates.end())
    {
        state->second->cancelAutoPause();
    }
}

void BsEcdhPSIImpl::onTaskFinished(
    const std::string& _taskID, protocol::TaskStatus _status, BsEcdhResult::Ptr _result)
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onTaskFinished") << LOG_KV("taskID", _taskID)
                          << LOG_KV("status", toString(_status));
    {
        WriteGuard l(x_tasks);
        auto state = m_taskStates.find(_taskID);
        if (state != m_taskStates.end())
        {
            state->second->updateStatus(_status);
            state->second->setResult(std::move(_result));
        }
    }
}
