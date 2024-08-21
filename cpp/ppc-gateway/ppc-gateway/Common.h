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
 * @file Common.h
 * @author: shawnhe
 * @date 2022-10-23
 */

#pragma once

#include "ppc-framework/Common.h"
#include <bcos-utilities/BoostLog.h>
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <json/json.h>
#include <boost/algorithm/string/split.hpp>
#include <functional>

namespace ppc::gateway
{
#define GATEWAY_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GATEWAY]"

#define GATEWAY_WS_CLIENT_MODULE "m_gateway_websocket_client"
#define GATEWAY_WS_SERVER_MODULE "m_gateway_websocket_server"
#define GATEWAY_THREAD_POOL_MODULE "t_gateway"

#define SEND_MESSAGE_TO_FRONT_SUCCESS "success"
#define SEND_MESSAGE_TO_FRONT_ERROR "error"
#define SEND_MESSAGE_TO_FRONT_TIMEOUT "timeout"

#define SEND_MESSAGE_TO_FRONT_SUCCESS_CODE "E0000000000"
#define SEND_MESSAGE_TO_FRONT_ERROR_CODE "-1"

//HTTP HEADER DEFINE
#define HEAD_TASK_ID "x-ptp-session-id"
#define HEAD_ALGO_TYPE "x-ptp-algorithm-type"
#define HEAD_TASK_TYPE "x-ptp-task-type"
#define HEAD_SENDER_ID "x-ptp-sender-id"
#define HEAD_MESSAGE_TYPE "x-ptp-message-type"
#define HEAD_IS_RESPONSE "x-ptp-is-response"
#define HEAD_SEQ "x-ptp-seq"
#define HEAD_UUID "x-ptp-uuid"
}  // namespace ppc::gateway