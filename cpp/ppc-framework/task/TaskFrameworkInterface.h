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
 * @brief the TaskFrameworkInterface
 * @file TaskFrameworkInterface.h
 * @author: yujiechen
 * @date 2022-11-16
 */
#pragma once
#include "../protocol/PPCMessageFace.h"
#include "../protocol/Task.h"
#include <bcos-utilities/Error.h>
#include <memory>
namespace ppc::task
{
using TaskResponseCallback = std::function<void(ppc::protocol::TaskResult::Ptr&&)>;

class TaskFrameworkInterface
{
public:
    using Ptr = std::shared_ptr<TaskFrameworkInterface>;
    TaskFrameworkInterface() = default;
    virtual ~TaskFrameworkInterface() = default;

    // run task
    virtual void asyncRunTask(
        ppc::protocol::Task::ConstPtr _task, TaskResponseCallback&& _onTaskFinished) = 0;

    // register to the front to get the message related to ra2018
    virtual void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg) = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
};
}  // namespace ppc::psi