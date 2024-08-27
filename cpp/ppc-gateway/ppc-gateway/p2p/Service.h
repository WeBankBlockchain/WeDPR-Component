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
 * @file Service.h
 * @author: yujiechen
 * @date 2024-08-26
 */

#pragma once
#include "../Common.h"
#include "ppc-framework/protocol/Message.h"
#include "router/RouterTableInterface.h"
#include <bcos-boostssl/websocket/WsService.h>
namespace ppc::gateway
{
class Service : public bcos::boostssl::ws::WsService
{
public:
    using Ptr = std::shared_ptr<Service>;
    Service(std::string const& _nodeID, RouterTableFactory::Ptr const& _routerTableFactory,
        int unreachableDistance, std::string _moduleName = "DEFAULT");

    virtual void asyncSendMessageByNodeID(std::string const& dstNodeID,
        bcos::boostssl::MessageFace::Ptr msg,
        bcos::boostssl::ws::Options options = bcos::boostssl::ws::Options(),
        bcos::boostssl::ws::RespCallBack respFunc = bcos::boostssl::ws::RespCallBack());

    virtual void asyncSendMessageByP2PNodeID(uint16_t packetType, std::string const& dstNodeID,
        std::shared_ptr<bcos::bytes> payload,
        bcos::boostssl::ws::Options options = bcos::boostssl::ws::Options(),
        bcos::boostssl::ws::RespCallBack callback = bcos::boostssl::ws::RespCallBack());

    virtual void asyncBroadcastMessage(bcos::boostssl::MessageFace::Ptr msg,
        bcos::boostssl::ws::Options options = bcos::boostssl::ws::Options());

    virtual void sendRespMessageBySession(bcos::boostssl::ws::WsSession::Ptr const& session,
        bcos::boostssl::MessageFace::Ptr msg, std::shared_ptr<bcos::bytes>&& payload);

    RouterTableFactory::Ptr const& routerTableFactory() const { return m_routerTableFactory; }
    RouterTableInterface::Ptr const& routerTable() const { return m_routerTable; }

    std::string const& nodeID() const { return m_nodeID; }
    bcos::boostssl::MessageFaceFactory::Ptr const& messageFactory() const
    {
        return m_messageFactory;
    }

protected:
    void onRecvMessage(bcos::boostssl::MessageFace::Ptr _msg,
        bcos::boostssl::ws::WsSession::Ptr _session) override;

    virtual void onP2PConnect(bcos::boostssl::ws::WsSession::Ptr _session);
    virtual void onP2PDisconnect(bcos::boostssl::ws::WsSession::Ptr _session);

    void reconnect() override;

    void updateNodeIDInfo(bcos::boostssl::ws::WsSession::Ptr const& _session);
    void removeSessionInfo(bcos::boostssl::ws::WsSession::Ptr const& _session);
    bcos::boostssl::ws::WsSession::Ptr getSessionByNodeID(std::string const& _nodeID);

    virtual void asyncSendMessageWithForward(std::string const& dstNodeID,
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::Options options,
        bcos::boostssl::ws::RespCallBack respFunc);

    virtual void asyncSendMessage(std::string const& dstNodeID,
        bcos::boostssl::MessageFace::Ptr msg, bcos::boostssl::ws::Options options,
        bcos::boostssl::ws::RespCallBack respFunc);

protected:
    std::string m_nodeID;
    // nodeID=>session
    std::unordered_map<std::string, bcos::boostssl::ws::WsSession::Ptr> m_nodeID2Session;
    bcos::RecursiveMutex x_nodeID2Session;

    RouterTableFactory::Ptr m_routerTableFactory;
    RouterTableInterface::Ptr m_routerTable;

    // configuredNode=>nodeID
    std::map<bcos::boostssl::NodeIPEndpoint, std::string> m_configuredNode2ID;
    mutable bcos::SharedMutex x_configuredNode2ID;
};
}  // namespace ppc::gateway