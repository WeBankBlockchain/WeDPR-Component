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
 * @file PeerRouterTable.h
 * @author: yujiechen
 * @date 2024-08-27
 */
#pragma once
#include "GatewayNodeInfo.h"
#include "ppc-framework/protocol/Message.h"
#include "ppc-framework/protocol/RouteType.h"
#include "ppc-gateway/p2p/Service.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::gateway
{
class PeerRouterTable
{
public:
    using Ptr = std::shared_ptr<PeerRouterTable>;
    PeerRouterTable(Service::Ptr service) : m_service(std::move(service)) {}
    virtual ~PeerRouterTable() = default;

    virtual void updateGatewayInfo(GatewayNodeInfo::Ptr const& gatewayInfo);
    virtual GatewayNodeInfos selectRouter(
        ppc::protocol::RouteType const& routeType, ppc::protocol::Message::Ptr const& msg) const;

    virtual void asyncBroadcastMessage(ppc::protocol::Message::Ptr const& msg) const;

private:
    virtual GatewayNodeInfos selectRouterByNodeID(ppc::protocol::Message::Ptr const& msg) const;
    virtual GatewayNodeInfos selectRouterByComponent(ppc::protocol::Message::Ptr const& msg) const;
    virtual GatewayNodeInfos selectRouterByAgency(ppc::protocol::Message::Ptr const& msg) const;


private:
    Service::Ptr m_service;
    // nodeID => p2pNodes
    std::map<bcos::bytes, GatewayNodeInfos> m_nodeID2GatewayInfos;
    // agency => p2pNodes
    std::map<std::string, GatewayNodeInfos> m_agency2GatewayInfos;
    mutable bcos::SharedMutex x_mutex;
};
}  // namespace ppc::gateway