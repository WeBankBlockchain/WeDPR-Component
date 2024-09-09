/*
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
 * @file RpcTypeDef.h
 * @author: yujiechen
 * @date 2022-11-3
 */
#pragma once
#include <json/json.h>

namespace ppc::rpc
{
enum class RpcError : int32_t
{
    LackOfMemory = -33,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    NotImplemented = -32602,
    StorageNotSet = -32603,
    NoPermission = -32604,
    BsModePsiNotSet = -32605,
};

std::string const RUN_TASK_METHOD = "runTask";
std::string const ASYNC_RUN_TASK_METHOD = "asyncRunTask";
std::string const GET_TASK_STATUS = "getTaskStatus";
std::string const GET_PEERS = "getPeers";

std::string const ASYNC_RUN_BS_MODE_TASK = "asyncRunBsModeTask";
std::string const FETCH_CIPHER = "fetchCipher";
std::string const SEND_ECDH_CIPHER = "sendEcdhCipher";
std::string const SEND_PARTNER_CIPHER = "sendPartnerCipher";
std::string const GET_BS_MODE_TASK_STATUS = "getBsModeTaskStatus";
std::string const KILL_BS_MODE_TASK = "killBsModeTask";
std::string const UPDATE_BS_MODE_TASK_STATUS = "updateBsModeTaskStatus";
}  // namespace ppc::rpc