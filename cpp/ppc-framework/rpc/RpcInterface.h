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
 * @file RpcInterface.h
 * @author: yujiechen
 * @date 2022-11-3
 */
#pragma once
#include "../protocol/Protocol.h"
#include "../protocol/Task.h"
#include <bcos-boostssl/httpserver/Common.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <json/json.h>
#include <memory>

#define RPC_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("RPC")

namespace bcos
{
namespace boostssl
{
class MessageFace;
class WsSession;
}  // namespace boostssl
}  // namespace bcos
namespace ppc::rpc
{
using RespFunc = std::function<void(bcos::Error::Ptr, Json::Value&&)>;
using TaskHandler = std::function<void(
    ppc::protocol::Task::ConstPtr const&, std::function<void(ppc::protocol::TaskResult::Ptr&&)>)>;
class RpcInterface
{
public:
    using Ptr = std::shared_ptr<RpcInterface>;
    RpcInterface() = default;
    virtual ~RpcInterface() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void registerHandler(std::string const& _methodName,
        std::function<void(Json::Value const&, RespFunc)> _handler) = 0;

    virtual void registerTaskHandler(
        ppc::protocol::TaskType _type, uint8_t _algorithm, TaskHandler _handler) = 0;
};
}  // namespace ppc::rpc