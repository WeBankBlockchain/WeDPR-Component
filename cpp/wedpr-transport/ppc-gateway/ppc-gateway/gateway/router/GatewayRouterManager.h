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
 * @file GatewayRouterManager.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-gateway/gateway/router/GatewayNodeInfo.h"
#include "ppc-gateway/gateway/router/LocalRouter.h"
#include "ppc-gateway/gateway/router/PeerRouterTable.h"
#include "ppc-gateway/p2p/Service.h"
#include <bcos-utilities/Timer.h>
#include <memory>

namespace ppc::gateway
{
class GatewayRouterManager
{
public:
    using Ptr = std::shared_ptr<GatewayRouterManager>;
    GatewayRouterManager(Service::Ptr service, GatewayNodeInfoFactory::Ptr nodeStatusFactory,
        LocalRouter::Ptr localRouter, PeerRouterTable::Ptr peerRouter,
        uint16_t seqSyncPeriod = 5000);
    virtual void start();
    virtual void stop();

    void removeUnreachableP2pNode(std::string const& p2pNode);

protected:
    virtual void onReceiveNodeSeqMessage(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);

    virtual void onReceiveRequestNodeStatusMsg(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);

    virtual void onRecvResponseNodeStatusMsg(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);
    bool statusChanged(std::string const& p2pNodeID, uint32_t seq);
    void broadcastStatusSeq();

    void updatePeerNodeStatus(std::string const& p2pID, GatewayNodeInfo::Ptr status);

private:
    Service::Ptr m_service;
    GatewayNodeInfoFactory::Ptr m_nodeStatusFactory;
    std::shared_ptr<bcos::Timer> m_timer;

    LocalRouter::Ptr m_localRouter;
    PeerRouterTable::Ptr m_peerRouter;

    bool m_running = false;

    // P2pID => statusSeq
    std::map<std::string, uint32_t> m_p2pID2Seq;
    mutable bcos::SharedMutex x_p2pID2Seq;

    uint16_t m_seqSyncPeriod;
};
}  // namespace ppc::gateway