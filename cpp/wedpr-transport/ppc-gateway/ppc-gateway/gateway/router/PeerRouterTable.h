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
    PeerRouterTable(Service::Ptr service, GatewayNodeInfoFactory::Ptr gatewayInfoFactory)
      : m_service(std::move(service)), m_gatewayInfoFactory(std::move(gatewayInfoFactory))
    {}
    virtual ~PeerRouterTable() = default;

    virtual void updateGatewayInfo(GatewayNodeInfo::Ptr const& gatewayInfo);
    virtual void removeP2PID(std::string const& p2pNode);
    virtual GatewayNodeInfos selectRouter(ppc::protocol::RouteType const& routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) const;

    virtual std::vector<std::string> selectTargetNodes(ppc::protocol::RouteType const& routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) const;

    virtual void asyncBroadcastMessage(ppc::protocol::Message::Ptr const& msg) const;

    std::set<std::string> agencies(std::vector<std::string> const& components) const;

    std::map<std::string, GatewayNodeInfos> gatewayInfos() const
    {
        bcos::ReadGuard l(x_mutex);
        return m_agency2GatewayInfos;
    }


private:
    virtual GatewayNodeInfos selectRouterByNodeID(
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) const;
    virtual GatewayNodeInfos selectRouterByComponent(
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) const;
    void selectRouterByComponent(GatewayNodeInfos& choosedGateway,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo,
        GatewayNodeInfos const& singleAgencyGatewayInfos) const;
    virtual GatewayNodeInfos selectRouterByAgency(
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) const;
    void removeP2PNodeIDFromNodeIDInfos(GatewayNodeInfo::Ptr const& gatewayInfo);
    void insertGatewayInfo(GatewayNodeInfo::Ptr const& gatewayInfo);
    void removeP2PNodeIDFromAgencyInfos(std::string const& p2pNode);

private:
    Service::Ptr m_service;
    GatewayNodeInfoFactory::Ptr m_gatewayInfoFactory;
    // nodeID => p2pNodes
    std::map<bcos::bytes, GatewayNodeInfos> m_nodeID2GatewayInfos;
    // agency => p2pNodes
    std::map<std::string, GatewayNodeInfos> m_agency2GatewayInfos;
    mutable bcos::SharedMutex x_mutex;
};
}  // namespace ppc::gateway