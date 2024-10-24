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
 * @file SendMessageWithRetry.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-framework/protocol/P2PMessage.h"
#include "ppc-gateway/gateway/router/GatewayNodeInfo.h"
#include "ppc-gateway/p2p/Service.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::gateway
{
class SendMessageWithRetry : public std::enable_shared_from_this<SendMessageWithRetry>
{
public:
    using Ptr = std::shared_ptr<SendMessageWithRetry>;
    SendMessageWithRetry(Service::Ptr const& service, GatewayNodeInfos&& dstNodeList,
        ppc::protocol::P2PMessage::Ptr&& p2pMessage, ppc::protocol::ReceiveMsgFunc respFunc,
        long timeout)
      : m_service(service),
        m_dstNodeList(std::move(dstNodeList)),
        m_p2pMessage(std::move(p2pMessage)),
        m_respFunc(std::move(respFunc)),
        m_timeout(timeout)
    {
        if (m_timeout < 0)
        {
            m_timeout = 10000;
        }
    }
    // random choose one p2pID to send message
    GatewayNodeInfo::Ptr chooseP2pNode();

    // send the message with retry
    void trySendMessage();

private:
    // mutex for p2pIDs
    mutable bcos::RecursiveMutex x_mutex;
    GatewayNodeInfos m_dstNodeList;
    ppc::protocol::P2PMessage::Ptr m_p2pMessage;
    Service::Ptr m_service;
    ppc::protocol::ReceiveMsgFunc m_respFunc;
    long m_timeout;
};
}  // namespace ppc::gateway