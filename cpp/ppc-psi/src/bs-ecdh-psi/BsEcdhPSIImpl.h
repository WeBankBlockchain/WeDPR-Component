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
 * @file BsEcdhPSIImpl.h
 * @author: shawnhe
 * @date 2023-09-20
 */

#pragma once

#include "BsEcdhPSIInterface.h"
#include "Common.h"
#include "core/BsEcdhCache.h"
#include "core/BsEcdhIoHandler.h"
#include "core/BsEcdhTaskState.h"
#include "ppc-psi/src/psi-framework/TaskGuarder.h"
#include <bcos-utilities/CallbackCollectionHandler.h>
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/ThreadPool.h>
#include <bcos-utilities/Timer.h>
#include <memory>
#include <queue>

namespace ppc::psi
{
class BsEcdhPSIImpl : public BsEcdhPSIInterface, public std::enable_shared_from_this<BsEcdhPSIImpl>
{
public:
    using Ptr = std::shared_ptr<BsEcdhPSIImpl>;

    BsEcdhPSIImpl(
        PSIConfig::Ptr _config, bcos::ThreadPool::Ptr _threadPool, uint32_t _timeoutMinutes)
      : m_config(std::move(_config)),
        m_threadPool(std::move(_threadPool)),
        m_taskCleaner(std::make_shared<bcos::Timer>(PAUSE_THRESHOLD, "BsEcdhTaskCleaner")),
        m_timeoutMinutes(_timeoutMinutes)
    {
        m_taskGuarder = std::make_shared<TaskGuarder>(m_config);
    }

    virtual ~BsEcdhPSIImpl() = default;

    BsEcdhResult::Ptr getTaskStatus(GetTaskStatusRequest::Ptr _request) override;

    virtual BsEcdhResult::Ptr updateTaskStatus(UpdateTaskStatusRequest::Ptr _request) override;

    virtual BsEcdhResult::Ptr killTask(KillTaskRequest::Ptr _request) override;

    void asyncRunTask(RunTaskRequest::Ptr _request,
        std::function<void(BsEcdhResult::Ptr&&)>&& _onTaskFinished) override;

    BsEcdhResult::Ptr fetchCipher(FetchCipherRequest::Ptr _request) override;

    BsEcdhResult::Ptr sendEcdhCipher(SendEcdhCipherRequest::Ptr _request) override;

    BsEcdhResult::Ptr sendPartnerCipher(SendPartnerCipherRequest::Ptr _request) override;

    void start() override;

    void stop() override;

    // allow the output-path exists, for ut
    void enableOutputExists() { m_enableOutputExists = true; }

protected:
    void checkAndCleanTask();
    std::pair<BsEcdhResult::Ptr, BsEcdhCache::Ptr> checkTaskRequest(const std::string& _taskID);
    void onSelfCiphersReady(const std::string& _taskID);
    void onAllCiphersReady(const std::string& _taskID);
    void onTaskFinished(
        const std::string& _taskID, protocol::TaskStatus _status, BsEcdhResult::Ptr _result);

    void addTask(BsEcdhTaskState::Ptr _state, BsEcdhCache::Ptr _cache)
    {
        bcos::WriteGuard l(x_tasks);
        m_taskStates[_state->taskID()] = std::move(_state);
        m_taskCaches[_cache->taskID()] = std::move(_cache);
    }

    BsEcdhCache::Ptr findTaskCache(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_tasks);
        auto cache = m_taskCaches.find(_taskID);
        if (cache == m_taskCaches.end())
        {
            return nullptr;
        }

        return cache->second;
    }

    BsEcdhTaskState::Ptr findTaskState(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_tasks);
        auto state = m_taskStates.find(_taskID);
        if (state == m_taskStates.end())
        {
            return nullptr;
        }

        return state->second;
    }

    uint32_t taskCount()
    {
        bcos::ReadGuard l(x_tasks);
        uint32_t count = 0;
        for (auto& state : m_taskStates)
        {
            if (state.second->status() != protocol::TaskStatus::COMPLETED &&
                state.second->status() != protocol::TaskStatus::FAILED)
            {
                count++;
            }
        }
        return count;
    }

    BsEcdhResult::Ptr prepareResultWithoutLock(
        const std::string& _taskID, const std::string& _status)
    {
        GetTaskStatusResponse response;
        response.taskID = _taskID;
        response.status = _status;

        auto cache = m_taskCaches.find(_taskID);
        if (cache != m_taskCaches.end())
        {
            response.step = cache->second->step();
            response.index = cache->second->index();
            response.progress = cache->second->progress();
        }
        return std::make_shared<BsEcdhResult>(_taskID, response.serialize());
    }

private:
    bool m_enableOutputExists = false;

    PSIConfig::Ptr m_config;
    bcos::ThreadPool::Ptr m_threadPool;
    std::shared_ptr<bcos::Timer> m_taskCleaner;
    uint32_t m_timeoutMinutes;
    TaskGuarder::Ptr m_taskGuarder;

    std::unordered_map<std::string, BsEcdhCache::Ptr> m_taskCaches;
    std::unordered_map<std::string, BsEcdhTaskState::Ptr> m_taskStates;
    mutable bcos::SharedMutex x_tasks;

    std::atomic<bool> m_started{false};
};

}  // namespace ppc::psi