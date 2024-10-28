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
 * @date 2022-10-25
 */

#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/BoostLog.h>
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/IOServicePool.h>
#include <bcos-utilities/ThreadPool.h>
#include <json/json.h>
#include <boost/algorithm/string/split.hpp>
#include <functional>

namespace ppc::front
{
#define FRONT_LOG(LEVEL) BCOS_LOG(LEVEL) << "[FRONT]"

#define HOLDING_MESSAGE_TIMEOUT_M 30
#define CHANNEL_THREAD_POOL_MODULE "t_channel_manager"

}  // namespace ppc::front