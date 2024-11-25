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
 * @file RpcStatusInterface.h
 * @author: shawnhe
 * @date 2023-02-19
 */

#pragma once
#include "../protocol/Protocol.h"
#include "../protocol/Task.h"
#include "../storage/SQLStorage.h"
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <json/json.h>
#include <memory>

#define RPC_STATUS_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("RpcStatus")

namespace ppc::rpc
{
class RpcStatusInterface
{
public:
    using Ptr = std::shared_ptr<RpcStatusInterface>;
    RpcStatusInterface() = default;
    virtual ~RpcStatusInterface() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual bcos::Error::Ptr insertTask(protocol::Task::Ptr _task) = 0;
    virtual bcos::Error::Ptr updateTaskStatus(protocol::TaskResult::Ptr _taskResult) = 0;
    virtual protocol::TaskResult::Ptr getTaskStatus(const std::string& _taskID) = 0;
};
}  // namespace ppc::rpc