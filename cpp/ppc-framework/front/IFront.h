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
 * @file IFront.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#pragma once
#include "FrontConfig.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include "ppc-framework/protocol/Message.h"
#include "ppc-framework/protocol/RouteType.h"
#include <bcos-utilities/Error.h>

namespace ppc::front
{
class IFrontClient
{
public:
    using Ptr = std::shared_ptr<IFrontClient>;
    IFrontClient() = default;
    virtual ~IFrontClient() = default;
    /**
     * @brief: receive message from gateway, call by gateway
     * @param _message: received ppc message
     * @return void
     */
    virtual void onReceiveMessage(
        ppc::protocol::Message::Ptr const& _msg, ppc::protocol::ReceiveMsgFunc _callback) = 0;
};
class IFront : public virtual IFrontClient
{
public:
    using Ptr = std::shared_ptr<IFront>;

    IFront() = default;
    ~IFront() override = default;

    /**
     * @brief start the IFront
     *
     * @param front the IFront to start
     */
    virtual void start() = 0;
    /**
     * @brief stop the IFront
     *
     * @param front the IFront to stop
     */
    virtual void stop() = 0;

    /**
     *
     * @param front the front object
     * @param topic the topic
     * @param callback the callback called when receive specified topic
     */
    virtual void registerTopicHandler(
        std::string const& topic, ppc::protocol::MessageDispatcherCallback callback) = 0;

    /**
     * @brief async send message
     *
     * @param routeType the route type
     * @param routeInfo the route info, include
     *  - topic  the topic
     *  - dstInst the dst agency(must set when 'route by agency' and 'route by
     * component')
     *  - dstNodeID  the dst nodeID(must set when 'route by nodeID')
     *  - componentType the componentType(must set when 'route by component')
     * @param payload the payload to send
     * @param seq the message seq
     * @param timeout timeout
     * @param callback callback
     */
    virtual void asyncSendMessage(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload, int seq,
        long timeout, ppc::protocol::ReceiveMsgFunc errorCallback,
        ppc::protocol::MessageCallback callback) = 0;

    // the sync interface for async_send_message
    virtual bcos::Error::Ptr push(ppc::protocol::RouteType routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload, int seq,
        long timeout) = 0;

    virtual ppc::protocol::Message::Ptr pop(std::string const& topic, long timeoutMs) = 0;
    virtual ppc::protocol::Message::Ptr peek(std::string const& topic) = 0;


    /**
     * @brief register the nodeInfo to the gateway
     * @param nodeInfo the nodeInfo
     */
    virtual void registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) = 0;

    /**
     * @brief unRegister the nodeInfo to the gateway
     */
    virtual void unRegisterNodeInfo() = 0;

    /**
     * @brief register the topic
     *
     * @param topic the topic to register
     */
    virtual void registerTopic(std::string const& topic) = 0;

    /**
     * @brief unRegister the topic
     *
     * @param topic the topic to unregister
     */
    virtual void unRegisterTopic(std::string const& topic) = 0;
};

class IFrontBuilder
{
public:
    using Ptr = std::shared_ptr<IFrontBuilder>;
    IFrontBuilder() = default;
    virtual ~IFrontBuilder() = default;

    /**
     * @brief create the Front using specified config
     *
     * @param config the config used to build the Front
     * @return IFront::Ptr he created Front
     */
    virtual IFront::Ptr build(ppc::front::FrontConfig::Ptr config) const = 0;
    virtual IFrontClient::Ptr buildClient(std::string endPoint) const = 0;
};
}  // namespace ppc::front