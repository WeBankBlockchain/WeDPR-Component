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
 * @file RpcMemory.h
 * @author: shawnhe
 * @date 2023-03-27
 * @desc: 如果无法启用mysqlx协议，则数据保存到内存中
 */

#pragma once
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/rpc/RpcStatusInterface.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Timer.h>

namespace ppc::rpc
{
class RpcMemory : public RpcStatusInterface
{
public:
    using Ptr = std::shared_ptr<RpcMemory>;

    RpcMemory(ppc::front::FrontInterface::Ptr front)
      : m_front(std::move(front)),
        m_taskCleaner(std::make_shared<bcos::Timer>(60 * 60 * 1000, "taskCleaner"))
    {}
    ~RpcMemory() override = default;

    void start() override;
    void stop() override;

    bcos::Error::Ptr insertTask(protocol::Task::Ptr _task) override;
    bcos::Error::Ptr updateTaskStatus(protocol::TaskResult::Ptr _taskResult) override;
    protocol::TaskResult::Ptr getTaskStatus(const std::string& _taskID) override;

protected:
    void cleanTask();

private:
    ppc::front::FrontInterface::Ptr m_front;

    mutable bcos::SharedMutex x_tasks;
    std::unordered_map<std::string, std::pair<uint64_t, protocol::TaskResult::Ptr>> m_tasks;
    std::shared_ptr<bcos::Timer> m_taskCleaner;

    const uint64_t VALIDITY_TERM = 24 * 60 * 60 * 1000;  // ms
};
}  // namespace ppc::rpc