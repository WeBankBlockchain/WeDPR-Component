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
#define SERVICE_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GATEWAY][SERVICE]"
#define SERVICE_ROUTER_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GATEWAY][SERVICE][ROUTER]"
#define LOCAL_ROUTER_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GATEWAY][LOCAL_ROUTER]"
#define PEER_ROUTER_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GATEWAY][PEER_ROUTER]"
#define ROUTER_MGR_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GATEWAY][ROUTER_MGR]"

// HTTP HEADER DEFINE
#define HEAD_TASK_ID "x-ptp-session-id"
#define HEAD_ALGO_TYPE "x-ptp-algorithm-type"
#define HEAD_TASK_TYPE "x-ptp-task-type"
#define HEAD_SENDER_ID "x-ptp-sender-id"
#define HEAD_MESSAGE_TYPE "x-ptp-message-type"
#define HEAD_IS_RESPONSE "x-ptp-is-response"
#define HEAD_SEQ "x-ptp-seq"
#define HEAD_UUID "x-ptp-uuid"
}  // namespace ppc::gateway