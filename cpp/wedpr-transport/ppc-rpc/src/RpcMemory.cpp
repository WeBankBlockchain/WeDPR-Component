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
 * @file RpcMemory.cpp
 * @author: shawnhe
 * @date 2023-03-28
 */

#include "RpcMemory.h"
#include "bcos-utilities/Common.h"

using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::rpc;
using namespace ppc::storage;

void RpcMemory::start()
{
    if (m_taskCleaner)
    {
        m_taskCleaner->registerTimeoutHandler(boost::bind(&RpcMemory::cleanTask, this));
        m_taskCleaner->start();
    }
}


void RpcMemory::stop()
{
    if (m_taskCleaner)
    {
        m_taskCleaner->stop();
    }
}

void RpcMemory::cleanTask()
{
    try
    {
        WriteGuard l(x_tasks);
        for (auto it = m_tasks.begin(); it != m_tasks.end();)
        {
            if (it->second.first + VALIDITY_TERM <= utcSteadyTime())
            {
                it = m_tasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    catch (std::exception const& e)
    {
        RPC_STATUS_LOG(WARNING) << LOG_DESC("cleanTask exception")
                                << LOG_KV("exception", boost::diagnostic_information(e));
    }
    m_taskCleaner->restart();
}

bcos::Error::Ptr RpcMemory::insertTask(protocol::Task::Ptr _task)
{
    WriteGuard l(x_tasks);
    auto it = m_tasks.find(_task->id());
    if (it != m_tasks.end())
    {
        auto taskResult = it->second.second;
        // the task already exists case
        if (!taskResult || taskResult->status() == toString(TaskStatus::RUNNING))
        {
            return std::make_shared<bcos::Error>(PPCRetCode::WRITE_RPC_STATUS_ERROR, "task exists");
        }
        if (taskResult)
        {
            RPC_STATUS_LOG(INFO) << LOG_DESC("find the existed not running-task")
                                 << LOG_KV("task", _task->id())
                                 << LOG_KV("status", taskResult->status());
            if (taskResult->status() != toString(TaskState::COMPLETED))
            {
                // erase the task_id
                m_front->eraseTaskInfo(_task->id());
            }
        }
    }
    auto taskResult = std::make_shared<TaskResult>(_task->id());
    taskResult->setStatus(toString(TaskStatus::RUNNING));
    m_tasks[_task->id()] = {utcSteadyTime(), std::move(taskResult)};
    return nullptr;
}

bcos::Error::Ptr RpcMemory::updateTaskStatus(protocol::TaskResult::Ptr _taskResult)
{
    WriteGuard l(x_tasks);
    if (m_tasks.find(_taskResult->taskID()) == m_tasks.end())
    {
        return std::make_shared<bcos::Error>(PPCRetCode::WRITE_RPC_STATUS_ERROR, "task not found");
    }
    m_tasks[_taskResult->taskID()].second = std::move(_taskResult);
    return nullptr;
}

TaskResult::Ptr RpcMemory::getTaskStatus(const std::string& _taskID)
{
    ReadGuard l(x_tasks);
    if (m_tasks.find(_taskID) == m_tasks.end())
    {
        auto error =
            std::make_shared<bcos::Error>(PPCRetCode::READ_RPC_STATUS_ERROR, "task not found");
        auto result = std::make_shared<TaskResult>(_taskID);
        result->setStatus(toString(TaskStatus::FAILED));
        result->setError(error);
        return result;
    }

    return m_tasks[_taskID].second;
}