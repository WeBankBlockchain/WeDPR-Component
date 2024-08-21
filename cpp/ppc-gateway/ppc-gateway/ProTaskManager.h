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
 * @file ProTaskManager.h
 * @author: shawnhe
 * @date 2022-10-23
 */
#pragma once
#include "TaskManager.h"

#include "ppc-framework/storage/CacheStorage.h"
#include <utility>
namespace ppc::gateway
{
class ProTaskManager : public TaskManager
{
public:
    using Ptr = std::shared_ptr<ProTaskManager>;
    ProTaskManager(
        storage::CacheStorage::Ptr _cache, std::shared_ptr<boost::asio::io_service> _ioService)
      : TaskManager(std::move(_ioService)), m_cache(std::move(_cache))
    {}
    ~ProTaskManager() override = default;

    void registerTaskInfo(const std::string& _taskID, const std::string& _serviceEndpoint) override
    {
        // throw exception if taskID existed
        TaskManager::registerTaskInfo(_taskID, _serviceEndpoint);
        try
        {
            // add task info to cache server
            m_cache->setValue(_taskID, _serviceEndpoint, TASK_TIMEOUT_M * 60);
        }
        catch (std::exception const& e)
        {
            GATEWAY_LOG(WARNING) << LOG_DESC(
                "set value failed: " + std::string(boost::diagnostic_information(e)));
        }
    }

    void removeTaskInfo(const std::string& _taskID) override
    {
        // Note: remove the memory-task-info in-case-of the redis exception
        TaskManager::removeTaskInfo(_taskID);
        m_cache->deleteKey(_taskID);
    }

    std::string getServiceEndpoint(const std::string& _taskID) override
    {
        // find task info in memory first
        try
        {
            auto endPoint = TaskManager::getServiceEndpoint(_taskID);
            if (!endPoint.empty())
            {
                return endPoint;
            }
            // Note: different node should not share the cache with same database
            // find task info in cache service
            auto serviceEndpoint = m_cache->getValue(_taskID);
            if (serviceEndpoint == std::nullopt)
            {
                GATEWAY_LOG(ERROR) << LOG_BADGE("keyNotFoundInCache") << LOG_KV("key", _taskID);
                return "";
            }
            GATEWAY_LOG(TRACE) << LOG_DESC("getServiceEndpoint: find the task from redis cache")
                               << LOG_KV("task", _taskID);
            // add task info to memory
            auto taskInfo = prepareTaskInfo(_taskID, *serviceEndpoint);
            addTaskInfo(_taskID, taskInfo);
            return *serviceEndpoint;
        }
        catch (std::exception const& e)
        {
            GATEWAY_LOG(ERROR) << LOG_DESC("getServiceEndpoint error")
                               << LOG_KV("exception", boost::diagnostic_information(e));
            return "";
        }
    }

private:
    storage::CacheStorage::Ptr m_cache;
};
}  // namespace ppc::gateway