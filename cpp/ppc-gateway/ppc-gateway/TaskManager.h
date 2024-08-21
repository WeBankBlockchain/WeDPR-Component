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
 * @file TaskManager.h
 * @author: shawnhe
 * @date 2022-10-23
 */

#pragma once

#include "Common.h"
#include "GatewayConfigContext.h"
#include <bcos-boostssl/websocket/WsService.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace ppc::gateway
{
class TaskManager : public std::enable_shared_from_this<TaskManager>
{
public:
    using Ptr = std::shared_ptr<TaskManager>;
    TaskManager(std::shared_ptr<boost::asio::io_service> _ioService)
      : m_ioService(std::move(_ioService))
    {}
    virtual ~TaskManager() = default;

    virtual void registerTaskInfo(const std::string& _taskID, const std::string& _serviceEndpoint);

    virtual std::string getServiceEndpoint(const std::string& _taskID);

    virtual void removeTaskInfo(const std::string& _taskID);

protected:
    struct TaskInfo
    {
        using Ptr = std::shared_ptr<TaskInfo>;
        std::string serviceEndpoint;
        // timeout of the task
        std::shared_ptr<boost::asio::deadline_timer> timer;
    };

    TaskInfo::Ptr prepareTaskInfo(const std::string& _taskID, const std::string& _serviceEndpoint);
    TaskInfo::Ptr getTaskInfo(const std::string& _taskID);
    void addTaskInfo(const std::string& _taskID, const TaskInfo::Ptr& _taskInfo);

protected:
    std::shared_ptr<boost::asio::io_service> m_ioService;
    // key: taskID, value: TaskInfo
    std::unordered_map<std::string, TaskInfo::Ptr> m_tasks;
    mutable bcos::SharedMutex x_tasks;

    constexpr static uint32_t TASK_TIMEOUT_M = 24 * 60;  // minutes
};
}  // namespace ppc::gateway
