/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file Message.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#pragma once
#include <stdint.h>

namespace ppc::gateway
{
enum class GatewayPacketType : uint16_t
{
    P2PMessage = 0x00,
    BroadcastMessage = 0x01,
    RouterTableSyncSeq = 0x10,
    RouterTableResponse = 0x11,
    RouterTableRequest = 0x12,
    SyncNodeSeq = 0x20,
    RequestNodeStatus = 0x21,
    ResponseNodeStatus = 0x22,
};

enum class GatewayMsgExtFlag : uint16_t
{
    Response = 0x1,
    RouteByNodeID = 0x2,
    RouteByAgency = 0x4,
    RouteByComponent = 0x8,
    RouteByTopic = 0x10
};

enum CommonError : int32_t
{
    SUCCESS = 0,
    TIMEOUT = 1000,  // for gateway
    NotFoundFrontServiceDispatchMsg = 1001
};
}  // namespace ppc::gateway