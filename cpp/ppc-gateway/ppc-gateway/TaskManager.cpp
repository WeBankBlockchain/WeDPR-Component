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
 * @file TaskManager.cpp
 * @author: shawnhe
 * @date 2022-10-23
 */

#include "TaskManager.h"
#include "ppc-framework/protocol/Protocol.h"

using namespace bcos;
using namespace ppc::gateway;
using namespace ppc::storage;

void TaskManager::registerTaskInfo(const std::string& _taskID, const std::string& _serviceEndpoint)
{
    GATEWAY_LOG(INFO) << LOG_BADGE("registerTaskInfo") << LOG_KV("taskID", _taskID)
                      << LOG_KV("serviceEndpoint", _serviceEndpoint);
    if (getTaskInfo(_taskID))
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR(protocol::PPCRetCode::EXCEPTION, "task id already exists"));
    }

    // add task info to memory
    auto taskInfo = prepareTaskInfo(_taskID, _serviceEndpoint);
    addTaskInfo(_taskID, taskInfo);
}


std::string TaskManager::getServiceEndpoint(const std::string& _taskID)
{
    // find task info in memory first
    auto taskInfo = getTaskInfo(_taskID);
    if (taskInfo)
    {
        return taskInfo->serviceEndpoint;
    }
    return "";
}


TaskManager::TaskInfo::Ptr TaskManager::prepareTaskInfo(
    const std::string& _taskID, const std::string& _serviceEndpoint)
{
    auto taskInfo = std::make_shared<TaskInfo>();
    taskInfo->serviceEndpoint = _serviceEndpoint;

    // create timer to handle timeout
    taskInfo->timer = std::make_shared<boost::asio::deadline_timer>(
        *m_ioService, boost::posix_time::minutes(TASK_TIMEOUT_M));

    taskInfo->timer->async_wait(
        [self = weak_from_this(), _taskID](boost::system::error_code _error) {
            if (!_error)
            {
                auto taskManager = self.lock();
                if (taskManager)
                {
                    // remove timeout event
                    taskManager->removeTaskInfo(_taskID);
                }
            }
        });

    return taskInfo;
}


TaskManager::TaskInfo::Ptr TaskManager::getTaskInfo(const std::string& _taskID)
{
    ReadGuard lock(x_tasks);
    auto it = m_tasks.find(_taskID);
    if (it != m_tasks.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}


void TaskManager::addTaskInfo(
    const std::string& _taskID, const TaskManager::TaskInfo::Ptr& _taskInfo)
{
    WriteGuard lock(x_tasks);
    GATEWAY_LOG(INFO) << LOG_BADGE("addTaskInfo") << LOG_KV("taskID", _taskID)
                      << LOG_KV("serviceEndpoint", _taskInfo->serviceEndpoint);
    m_tasks.emplace(_taskID, _taskInfo);
}


void TaskManager::removeTaskInfo(const std::string& _taskID)
{
    WriteGuard lock(x_tasks);
    GATEWAY_LOG(INFO) << LOG_BADGE("removeTaskInfo") << LOG_KV("taskID", _taskID);
    m_tasks.erase(_taskID);
}
