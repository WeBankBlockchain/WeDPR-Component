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
 * @file RouterManager.h
 * @author: yujiechen
 * @date 2024-08-26
 */

#pragma once
#include "../Service.h"
#include "RouterTableInterface.h"
#include <bcos-utilities/ThreadPool.h>
#include <bcos-utilities/Timer.h>

namespace ppc::gateway
{
class RouterManager : public std::enable_shared_from_this<RouterManager>
{
public:
    using Ptr = std::shared_ptr<RouterManager>;
    RouterManager(Service::Ptr service);
    virtual ~RouterManager() = default;

    // handlers called when the node is unreachable
    void registerUnreachableHandler(std::function<void(std::string)> _handler)
    {
        bcos::WriteGuard l(x_unreachableHandlers);
        m_unreachableHandlers.emplace_back(_handler);
    }

    virtual void start();
    virtual void stop();
    std::set<std::string> onEraseSession(std::string const& sessionNodeID);
    std::set<std::string> onNewSession(std::string const& sessionNodeID);

private:
    void onReceiveRouterSeq(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);
    bool tryToUpdateSeq(std::string const& _p2pNodeID, uint32_t _seq);
    void broadcastRouterSeq();

    void onReceivePeersRouterTable(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);
    void onReceiveRouterTableRequest(
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::WsSession::Ptr session);

    void joinRouterTable(std::string const& _generatedFrom, RouterTableInterface::Ptr _routerTable);
    bool eraseSeq(std::string const& _p2pNodeID);

    void onP2PNodesUnreachable(std::set<std::string> const& _p2pNodeIDs);

private:
    // for message forward
    Service::Ptr m_service;

    std::shared_ptr<bcos::Timer> m_routerTimer;

    // called when the given node unreachable
    std::vector<std::function<void(std::string)>> m_unreachableHandlers;
    mutable bcos::SharedMutex x_unreachableHandlers;

    std::map<std::string, uint32_t> m_node2Seq;
    mutable bcos::SharedMutex x_node2Seq;

    std::atomic<uint32_t> m_statusSeq{1};
};
}  // namespace ppc::gateway