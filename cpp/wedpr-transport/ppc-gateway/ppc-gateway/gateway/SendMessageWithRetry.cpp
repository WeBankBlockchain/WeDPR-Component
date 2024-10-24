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
#include "SendMessageWithRetry.h"
#include "ppc-framework/gateway/GatewayProtocol.h"
#include "ppc-gateway/Common.h"

using namespace bcos;
using namespace ppc;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;
using namespace ppc::gateway;
using namespace ppc::protocol;

// random choose one p2pID to send message
GatewayNodeInfo::Ptr SendMessageWithRetry::chooseP2pNode()
{
    RecursiveGuard lock(x_mutex);
    if (!m_dstNodeList.empty())
    {
        auto selectedNode = *(m_dstNodeList.begin());
        m_dstNodeList.erase(m_dstNodeList.begin());
        return selectedNode;
    }
    return nullptr;
}

// send the message with retry
void SendMessageWithRetry::trySendMessage()
{
    if (m_dstNodeList.empty())
    {
        GATEWAY_LOG(DEBUG) << LOG_DESC("Gateway::SendMessageWithRetry: unable to send the message")
                           << printP2PMessage(m_p2pMessage);
        if (m_respFunc)
        {
            m_respFunc(std::make_shared<bcos::Error>(
                -1, "can't find the gateway to send the message, detail: " +
                        printP2PMessage(m_p2pMessage)));
        }
        return;
    }
    auto choosedNode = chooseP2pNode();
    auto self = shared_from_this();
    auto startT = utcTime();
    auto callback = [self, startT](
                        bcos::Error::Ptr error, MessageFace::Ptr msg, WsSession::Ptr session) {
        std::ignore = session;
        if (error && error->errorCode() != 0)
        {
            GATEWAY_LOG(DEBUG) << LOG_BADGE("trySendMessage")
                               << LOG_DESC("send message failed, retry again")
                               << LOG_KV("msg", printP2PMessage(self->m_p2pMessage))
                               << LOG_KV("code", error->errorCode())
                               << LOG_KV("msg", error->errorMessage())
                               << LOG_KV("timeCost", (utcTime() - startT));
            // try again
            self->trySendMessage();
            return;
        }
        // check the errorCode
        try
        {
            auto payload = msg->payload();
            int respCode = boost::lexical_cast<int>(std::string(payload->begin(), payload->end()));
            // the peer gateway not response not ok ,it means the gateway not dispatch the
            // message successfully,find another gateway and try again
            if (respCode != CommonError::SUCCESS)
            {
                GATEWAY_LOG(DEBUG)
                    << LOG_BADGE("trySendMessage again") << LOG_KV("respCode", respCode)
                    << LOG_KV("msg", printP2PMessage(self->m_p2pMessage));
                // try again
                self->trySendMessage();
                return;
            }
            GATEWAY_LOG(TRACE) << LOG_BADGE("asyncSendMessageByNodeID success")
                               << LOG_KV("msg", printP2PMessage(self->m_p2pMessage));
            // send message successfully
            if (self->m_respFunc)
            {
                self->m_respFunc(nullptr);
            }
            return;
        }
        catch (const std::exception& e)
        {
            GATEWAY_LOG(ERROR) << LOG_BADGE("trySendMessage and receive response exception")
                               << LOG_KV("msg", printP2PMessage(self->m_p2pMessage))
                               << LOG_KV("error", boost::diagnostic_information(e));

            self->trySendMessage();
        }
    };
    // Note: make 10s configuarable here
    m_service->asyncSendMessageByNodeID(
        choosedNode->p2pNodeID(), m_p2pMessage, bcos::boostssl::ws::Options(m_timeout), callback);
}