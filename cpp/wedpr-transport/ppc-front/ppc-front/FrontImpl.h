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
 * @file FrontImpl.h
 * @author: yujiechen
 * @date 2024-08-30
 */
#pragma once
#include "CallbackManager.h"
#include "Common.h"
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/gateway/IGateway.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/ThreadPool.h>
#include <boost/asio/deadline_timer.hpp>

namespace ppc::front
{
class FrontImpl : public IFront, public IFrontClient, public std::enable_shared_from_this<FrontImpl>
{
public:
    using Ptr = std::shared_ptr<FrontImpl>;
    FrontImpl(std::shared_ptr<bcos::ThreadPool> threadPool, ppc::protocol::INodeInfo::Ptr nodeInfo,
        ppc::protocol::MessagePayloadBuilder::Ptr messageFactory,
        ppc::protocol::MessageOptionalHeaderBuilder::Ptr routerInfoBuilder,
        ppc::gateway::IGateway::Ptr const& gateway,
        std::shared_ptr<boost::asio::io_service> ioService);
    ~FrontImpl() override = default;

    /**
     * @brief start the IFront
     *
     * @param front the IFront to start
     */
    void start() override;
    /**
     * @brief stop the IFront
     *
     * @param front the IFront to stop
     */
    void stop() override;

    bcos::Error::Ptr push(uint16_t routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytesConstRef payload,
        int seq, long timeout) override;
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
    void asyncSendMessage(uint16_t routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytesConstRef payload,
        int seq, long timeout, ppc::protocol::ReceiveMsgFunc errorCallback,
        ppc::protocol::MessageCallback callback) override;

    /**
     * @brief: receive message from gateway, call by gateway
     * @param _message: received ppc message
     * @return void
     */
    void onReceiveMessage(
        ppc::protocol::Message::Ptr const& _msg, ppc::protocol::ReceiveMsgFunc _callback) override;

    ppc::protocol::Message::Ptr pop(std::string const& topic, long timeoutMs) override
    {
        return m_callbackManager->pop(topic, timeoutMs);
    }

    ppc::protocol::Message::Ptr peek(std::string const& topic) override
    {
        return m_callbackManager->pop(topic, 0);
    }
    /**
     *
     * @param front the front object
     * @param topic the topic
     * @param callback the callback called when receive specified topic
     */
    void registerTopicHandler(
        std::string const& topic, ppc::protocol::MessageDispatcherCallback callback) override
    {
        m_callbackManager->registerTopicHandler(topic, callback);
    }

    void registerMessageHandler(std::string const& componentType,
        ppc::protocol::MessageDispatcherCallback callback) override
    {
        m_callbackManager->registerMessageHandler(componentType, callback);
    }

    /**
     * @brief register the nodeInfo to the gateway
     * @param nodeInfo the nodeInfo
     */
    bcos::Error::Ptr registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) override
    {
        FRONT_LOG(INFO) << LOG_DESC("registerNodeInfo")
                        << LOG_KV("nodeInfo", printNodeInfo(m_nodeInfo));
        return m_gatewayClient->registerNodeInfo(m_nodeInfo);
    }

    /**
     * @brief unRegister the nodeInfo to the gateway
     */
    bcos::Error::Ptr unRegisterNodeInfo() override
    {
        FRONT_LOG(INFO) << LOG_DESC("unRegisterNodeInfo");
        return m_gatewayClient->unRegisterNodeInfo(bcos::ref(m_nodeID));
    }

    /**
     * @brief register the topic
     *
     * @param topic the topic to register
     */
    bcos::Error::Ptr registerTopic(std::string const& topic) override
    {
        FRONT_LOG(INFO) << LOG_DESC("register topic: ") << topic;
        return m_gatewayClient->registerTopic(bcos::ref(m_nodeID), topic);
    }

    void asyncGetAgencies(std::vector<std::string> const& components,
        std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback) override
    {
        m_gatewayClient->asyncGetAgencies(components, callback);
    }

    void asyncGetPeers(GetPeersInfoHandler::Ptr getPeersCallback) override;

    /**
     * @brief unRegister the topic
     *
     * @param topic the topic to unregister
     */
    bcos::Error::Ptr unRegisterTopic(std::string const& topic) override
    {
        FRONT_LOG(INFO) << LOG_DESC("unregister topic: ") << topic;
        return m_gatewayClient->unRegisterTopic(bcos::ref(m_nodeID), topic);
    }

    ppc::protocol::MessageOptionalHeaderBuilder::Ptr const routerInfoBuilder() const
    {
        return m_routerInfoBuilder;
    }
    ppc::protocol::MessagePayloadBuilder::Ptr const payloadFactory() const
    {
        return m_messageFactory;
    }

    void asyncSendResponse(bcos::bytesConstRef dstNode, std::string const& traceID,
        bcos::bytesConstRef payload, int seq, ppc::protocol::ReceiveMsgFunc errorCallback) override;

    ppc::protocol::INodeInfo::Ptr const& nodeInfo() override { return m_nodeInfo; }

    void registerComponent(std::string const& component) override;
    void unRegisterComponent(std::string const& component) override;

    std::vector<std::string> selectNodesByRoutePolicy(
        int16_t routeType, ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) override
    {
        return m_gatewayClient->selectNodesByRoutePolicy(
            (ppc::protocol::RouteType)routeType, routeInfo);
    }

private:
    void asyncSendMessageToGateway(bool responsePacket,
        ppc::protocol::MessagePayload::Ptr&& frontMessage, ppc::protocol::RouteType routeType,
        std::string const& traceID, ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo,
        long timeout, ppc::protocol::ReceiveMsgFunc callback);

    void handleCallback(bcos::Error::Ptr const& error, std::string const& traceID,
        ppc::protocol::Message::Ptr message);

private:
    bcos::bytes m_nodeID;
    std::shared_ptr<bcos::ThreadPool> m_threadPool;
    ppc::protocol::INodeInfo::Ptr m_nodeInfo;
    ppc::protocol::MessagePayloadBuilder::Ptr m_messageFactory;
    ppc::protocol::MessageOptionalHeaderBuilder::Ptr m_routerInfoBuilder;

    ppc::gateway::IGateway::Ptr m_gatewayClient;
    std::shared_ptr<boost::asio::io_service> m_ioService;

    CallbackManager::Ptr m_callbackManager;

    bool m_running = false;
    // the thread to run ioservice
    std::shared_ptr<std::thread> m_thread;
};
}  // namespace ppc::front