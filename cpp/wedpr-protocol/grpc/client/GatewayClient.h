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
 * @file GatewayClient.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "ppc-framework/gateway/IGateway.h"

namespace ppc::protocol
{
class GatewayClient : public ppc::gateway::IGateway
{
public:
    using Ptr = std::shared_ptr<GatewayClient>;
    GatewayClient() = default;
    ~GatewayClient() override = default;


    void start() override;
    void stop() override;

    /**
     * @brief send message to gateway
     *
     * @param routeType the route type
     * @param topic  the topic
     * @param dstInst the dst agency(must set when 'route by agency' and 'route by
     * component')
     * @param dstNodeID  the dst nodeID(must set when 'route by nodeID')
     * @param componentType the componentType(must set when 'route by component')
     * @param payload the payload to send
     * @param seq the message seq
     * @param timeout timeout
     * @param callback callback
     */
    void asyncSendMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload,
        long timeout, ppc::protocol::ReceiveMsgFunc callback) override;

    void asyncSendbroadcastMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload) override;
    void registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) override;
    void unRegisterNodeInfo(bcos::bytesConstRef nodeID) override;
    void registerTopic(bcos::bytesConstRef nodeID, std::string const& topic) override;
    void unRegisterTopic(bcos::bytesConstRef nodeID, std::string const& topic) override;
};
}  // namespace ppc::protocol