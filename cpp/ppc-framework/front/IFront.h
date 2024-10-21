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
#include "ppc-framework/protocol/Handler.h"
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

///////// the callback definition for sdk wrapper /////////
class ErrorCallback
{
public:
    using Ptr = std::shared_ptr<ErrorCallback>;
    ErrorCallback() = default;
    virtual ~ErrorCallback() {}

    virtual void onError(bcos::Error::Ptr error) = 0;
};

class MessageDispatcherHandler
{
public:
    using Ptr = std::shared_ptr<MessageDispatcherHandler>;
    MessageDispatcherHandler() = default;
    virtual ~MessageDispatcherHandler() {}

    virtual void onMessage(ppc::protocol::Message::Ptr msg) = 0;
};


class SendResponseHandler
{
public:
    using Ptr = std::shared_ptr<SendResponseHandler>;
    SendResponseHandler(ppc::protocol::SendResponseFunction responseFunc)
      : m_responseFunc(responseFunc)
    {}
    virtual ~SendResponseHandler() {}

    virtual void sendResponse(std::shared_ptr<bcos::bytes>&& payload)
    {
        m_responseFunc(std::move(payload));
    }

private:
    ppc::protocol::SendResponseFunction m_responseFunc;
};

class IMessageHandler
{
public:
    using Ptr = std::shared_ptr<IMessageHandler>;
    IMessageHandler() = default;
    virtual ~IMessageHandler() {}

    virtual void onMessage(bcos::Error::Ptr e, ppc::protocol::Message::Ptr msg,
        SendResponseHandler sendResponseHandler) = 0;
};

class GetPeersInfoHandler
{
public:
    using Ptr = std::shared_ptr<GetPeersInfoHandler>;
    GetPeersInfoHandler() = default;
    virtual ~GetPeersInfoHandler() {}

    virtual void onPeersInfo(bcos::Error::Ptr e, std::string const& peersInfo) = 0;
};

///////// the callback definition for sdk wrapper /////////

class IFront : virtual public IFrontClient
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

    /////// to simplify SDK wrapper  ////
    virtual void register_topic_handler(
        std::string const& topic, MessageDispatcherHandler::Ptr callback)
    {
        registerTopicHandler(topic, populateMessageDispatcherCallback(callback));
    }

    virtual void registerMessageHandler(
        std::string const& componentType, ppc::protocol::MessageDispatcherCallback callback) = 0;

    /////// to simplify SDK wrapper  ////
    virtual void register_msg_handler(
        std::string const& componentType, MessageDispatcherHandler::Ptr callback)
    {
        registerMessageHandler(componentType, populateMessageDispatcherCallback(callback));
    }

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
    virtual void asyncSendMessage(uint16_t routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload, int seq,
        long timeout, ppc::protocol::ReceiveMsgFunc errorCallback,
        ppc::protocol::MessageCallback callback) = 0;

    /////// to simplify SDK wrapper ////

    // !!! Note: the 'payload' type(char*) should not been changed, since it used to pass-in java
    // byte[] data
    virtual void async_send_message(uint16_t routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, char* payload,
        uint64_t payloadSize, int seq, long timeout, ErrorCallback::Ptr errorCallback,
        IMessageHandler::Ptr msgHandler)
    {
        // TODO: optimize here
        bcos::bytes copyedPayload(payload, payload + payloadSize);
        asyncSendMessage(routeType, routeInfo, std::move(copyedPayload), seq, timeout,
            populateErrorCallback(errorCallback), populateMsgCallback(msgHandler));
    }

    virtual void asyncSendResponse(bcos::bytes const& dstNode, std::string const& traceID,
        bcos::bytes&& payload, int seq, ppc::protocol::ReceiveMsgFunc errorCallback) = 0;

    /////// to simplify SDK wrapper  ////

    // !!! Note: the 'payload ' type(char*) should not been changed, since it used to pass-in java
    // byte[] data
    virtual void async_send_response(char* dstNode, uint64_t dstNodeSize,
        std::string const& traceID, char* payload, uint64_t payloadSize, int seq,
        ErrorCallback::Ptr errorCallback)
    {
        // TODO: optimize here
        bcos::bytes copiedDstNode(dstNode, dstNode + dstNodeSize);
        bcos::bytes copyedPayload(payload, payload + payloadSize);
        asyncSendResponse(copiedDstNode, traceID, std::move(copyedPayload), seq,
            populateErrorCallback(errorCallback));
    }

    // the sync interface for async_send_message
    virtual bcos::Error::Ptr push(uint16_t routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload, int seq,
        long timeout) = 0;

    // TODO: optmize here
    // Note: the python not support function overload, for different interfaces with the same
    // functionality, it is best to define methods with different names the 'payload', 'payloadSize'
    // should not been changed any more, since the swig has defined by the name to convert python
    // bytes to cpp (char*, uint64_t) %pybuffer_binary(char* payload, uint64_t payloadSize)
    virtual bcos::Error::Ptr push_msg(uint16_t routeType,
        ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo, char* payload,
        uint64_t payloadSize, int seq, long timeout)
    {
        bcos::bytes copyedPayload(payload, payload + payloadSize);
        return push(routeType, routeInfo, std::move(copyedPayload), seq, timeout);
    }
    virtual ppc::protocol::Message::Ptr pop(std::string const& topic, long timeoutMs) = 0;
    virtual ppc::protocol::Message::Ptr peek(std::string const& topic) = 0;

    virtual void asyncGetAgencies(std::vector<std::string> const& components,
        std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback) = 0;

    virtual void asyncGetPeers(GetPeersInfoHandler::Ptr getPeersCallback) = 0;

    /**
     * @brief register the nodeInfo to the gateway
     * @param nodeInfo the nodeInfo
     */
    virtual bcos::Error::Ptr registerNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) = 0;

    /**
     * @brief unRegister the nodeInfo to the gateway
     */
    virtual bcos::Error::Ptr unRegisterNodeInfo() = 0;

    virtual ppc::protocol::INodeInfo::Ptr const& nodeInfo() = 0;

    /**
     * @brief register the topic
     *
     * @param topic the topic to register
     */
    virtual bcos::Error::Ptr registerTopic(std::string const& topic) = 0;

    /**
     * @brief unRegister the topic
     *
     * @param topic the topic to unregister
     */
    virtual bcos::Error::Ptr unRegisterTopic(std::string const& topic) = 0;

    virtual void registerComponent(std::string const& component) = 0;
    virtual void unRegisterComponent(std::string const& component) = 0;

    // get the target nodeList according to the routeInfo
    virtual std::vector<std::string> selectNodesByRoutePolicy(
        int16_t routeType, ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo) = 0;

private:
    ppc::protocol::ReceiveMsgFunc populateErrorCallback(ErrorCallback::Ptr errorCallback)
    {
        if (errorCallback == nullptr)
        {
            return nullptr;
        }
        return [errorCallback](bcos::Error::Ptr error) { errorCallback->onError(error); };
    }

    ppc::protocol::MessageDispatcherCallback populateMessageDispatcherCallback(
        MessageDispatcherHandler::Ptr handler)
    {
        if (handler == nullptr)
        {
            return nullptr;
        }
        return [handler](ppc::protocol::Message::Ptr msg) { handler->onMessage(msg); };
    }

    ppc::protocol::MessageCallback populateMsgCallback(IMessageHandler::Ptr msgHandler)
    {
        if (msgHandler == nullptr)
        {
            return nullptr;
        }
        return [msgHandler](bcos::Error::Ptr e, ppc::protocol::Message::Ptr msg,
                   ppc::protocol::SendResponseFunction resFunc) {
            SendResponseHandler sendResponseHandler(resFunc);
            msgHandler->onMessage(e, msg, sendResponseHandler);
        };
    }
};

class IFrontBuilder
{
public:
    using Ptr = std::shared_ptr<IFrontBuilder>;
    IFrontBuilder() = default;
    virtual ~IFrontBuilder() = default;

    virtual IFrontClient::Ptr buildClient(std::string endPoint,
        std::function<void()> onUnHealthHandler, bool removeHandlerOnUnhealth) const = 0;
};
}  // namespace ppc::front