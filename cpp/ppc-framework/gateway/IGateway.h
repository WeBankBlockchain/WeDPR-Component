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
 * @file IGateway.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "../protocol/INodeInfo.h"
#include "../protocol/Message.h"
#include "../protocol/RouteType.h"
#include <bcos-utilities/Error.h>


namespace ppc::gateway
{
using ErrorCallbackFunc = std::function<void(bcos::Error::Ptr)>;
/**
 * @brief: A list of interfaces provided by the gateway which are called by the front service.
 */
class IGateway
{
public:
    using Ptr = std::shared_ptr<IGateway>;
    IGateway() = default;
    virtual ~IGateway() {}

    /**
     * @brief: start/stop service
     */
    virtual void start() = 0;
    virtual void stop() = 0;

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
    virtual void asyncSendMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID,
        bcos::bytes&& payload, long timeout, ppc::protocol::ReceiveMsgFunc callback) = 0;

    virtual void asyncSendbroadcastMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID,
        bcos::bytes&& payload) = 0;

    virtual void asyncGetPeers(std::function<void(bcos::Error::Ptr, std::string)> callback) = 0;
    virtual void asyncGetAgencies(std::vector<std::string> const& components,
        std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback) = 0;

    virtual bcos::Error::Ptr registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) = 0;
    virtual bcos::Error::Ptr unRegisterNodeInfo(bcos::bytesConstRef nodeID) = 0;
    virtual bcos::Error::Ptr registerTopic(
        bcos::bytesConstRef nodeID, std::string const& topic) = 0;
    virtual bcos::Error::Ptr unRegisterTopic(
        bcos::bytesConstRef nodeID, std::string const& topic) = 0;

    // get the target nodeList according to the routeInfo
    virtual std::vector<std::string> selectNodesByRoutePolicy(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) = 0;
};

}  // namespace ppc::gateway
