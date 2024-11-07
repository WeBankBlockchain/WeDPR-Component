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
 * @file FrontImpl.cpp
 * @author: yujiechen
 * @date 2024-08-30
 */
#include "FrontImpl.h"
#include "NodeDiscovery.h"
#include "ppc-utilities/Utilities.h"

using namespace bcos;
using namespace ppc;
using namespace ppc::front;
using namespace ppc::protocol;

FrontImpl::FrontImpl(std::shared_ptr<bcos::ThreadPool> threadPool,
    ppc::protocol::INodeInfo::Ptr nodeInfo, MessagePayloadBuilder::Ptr messageFactory,
    ppc::protocol::MessageOptionalHeaderBuilder::Ptr routerInfoBuilder,
    ppc::gateway::IGateway::Ptr const& gateway, std::shared_ptr<boost::asio::io_service> ioService)
  : m_threadPool(std::move(threadPool)),
    m_nodeInfo(std::move(nodeInfo)),
    m_messageFactory(std::move(messageFactory)),
    m_routerInfoBuilder(std::move(routerInfoBuilder)),
    m_ioService(std::move(ioService)),
    m_gatewayClient(gateway),
    m_nodeDiscovery(std::make_shared<NodeDiscovery>(gateway))
{
    m_nodeID = m_nodeInfo->nodeID().toBytes();
    m_callbackManager = std::make_shared<CallbackManager>(m_threadPool, m_ioService);
}


/**
 * @brief start the IFront
 *
 * @param front the IFront to start
 */
void FrontImpl::start()
{
    if (m_running)
    {
        FRONT_LOG(INFO) << LOG_DESC("The front has already been started");
        return;
    }
    m_running = true;
    m_thread = std::make_shared<std::thread>([&] {
        bcos::pthread_setThreadName("front_io_service");
        while (m_running)
        {
            try
            {
                m_ioService->run();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (m_running && m_ioService->stopped())
                {
                    m_ioService->restart();
                }
            }
            catch (std::exception& e)
            {
                FRONT_LOG(WARNING)
                    << LOG_DESC("Exception in Front Thread:") << boost::diagnostic_information(e);
            }
        }
        FRONT_LOG(INFO) << "Front exit";
    });
    if (m_nodeDiscovery)
    {
        m_nodeDiscovery->start();
    }
}


/**
 * @brief stop the IFront
 *
 * @param front the IFront to stop
 */
void FrontImpl::stop()
{
    if (!m_running)
    {
        FRONT_LOG(INFO) << LOG_DESC("The front has already been stopped");
        return;
    }
    m_running = false;
    if (m_ioService)
    {
        m_ioService->stop();
    }
    if (m_thread)
    {
        // stop the thread
        if (m_thread->get_id() != std::this_thread::get_id())
        {
            m_thread->join();
        }
        else
        {
            m_thread->detach();
        }
    }
    if (m_nodeDiscovery)
    {
        m_nodeDiscovery->stop();
    }
}

void FrontImpl::asyncSendResponse(bcos::bytesConstRef dstNode, std::string const& traceID,
    bcos::bytesConstRef payload, int seq, ppc::protocol::ReceiveMsgFunc errorCallback)
{
    // generate the frontMessage
    auto frontMessage = m_messageFactory->build();
    frontMessage->setTraceID(traceID);
    frontMessage->setSeq(seq);
    frontMessage->setDataPtr(payload);

    auto routeInfo = m_routerInfoBuilder->build();
    routeInfo->setSrcNode(m_nodeID);
    routeInfo->setDstNode(dstNode.toBytes());

    asyncSendMessageToGateway(true, std::move(frontMessage), RouteType::ROUTE_THROUGH_NODEID,
        traceID, routeInfo, -1, errorCallback);
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
void FrontImpl::asyncSendMessage(uint16_t routeType, MessageOptionalHeader::Ptr const& routeInfo,
    bcos::bytesConstRef payload, int seq, long timeout, ReceiveMsgFunc errorCallback,
    MessageCallback callback)
{
    // generate the frontMessage
    MessagePayload::Ptr frontMessage = m_messageFactory->build();
    auto traceID = ppc::generateUUID();
    frontMessage->setTraceID(traceID);
    frontMessage->setSeq(seq);
    frontMessage->setDataPtr(payload);
    m_callbackManager->addCallback(traceID, timeout, callback);
    auto self = weak_from_this();
    // send the message to the gateway
    asyncSendMessageToGateway(false, std::move(frontMessage), (ppc::protocol::RouteType)routeType,
        traceID, routeInfo, timeout,
        [self, traceID, routeInfo, errorCallback](bcos::Error::Ptr error) {
            auto front = self.lock();
            if (!front)
            {
                return;
            }
            // send success
            if (error && error->errorCode() != 0)
            {
                // send failed
                FRONT_LOG(WARNING)
                    << LOG_DESC("asyncSendMessage failed")
                    << LOG_KV("routeInfo", printOptionalField(routeInfo))
                    << LOG_KV("traceID", traceID) << LOG_KV("code", error->errorCode())
                    << LOG_KV("msg", error->errorMessage());
                // try to trigger the callback
                front->handleCallback(error, traceID, nullptr);
            }
            // Note: be careful block here when use push
            if (errorCallback)
            {
                errorCallback(error);
            }
        });
}

void FrontImpl::handleCallback(
    bcos::Error::Ptr const& error, std::string const& traceID, Message::Ptr message)
{
    auto self = weak_from_this();
    m_callbackManager->handleCallback(error, traceID, std::move(message),
        [self, message](std::shared_ptr<bcos::bytes>&& payload) {
            auto front = self.lock();
            if (!front)
            {
                return;
            }
            auto frontMessage = front->m_messageFactory->build();
            // set the traceID
            frontMessage->setTraceID(message->header()->traceID());
            ///// populate the route info
            auto routerInfo = front->m_routerInfoBuilder->build(message->header()->optionalField());
            // set the dstNodeID
            routerInfo->setDstNode(message->header()->optionalField()->srcNode());
            // set the srcNodeID
            routerInfo->setSrcNode(message->header()->optionalField()->dstNode());
            front->asyncSendMessageToGateway(true, std::move(frontMessage),
                RouteType::ROUTE_THROUGH_NODEID, message->header()->traceID(), routerInfo, 0,
                [routerInfo](bcos::Error::Ptr error) {
                    if (!error || error->errorCode() == 0)
                    {
                        return;
                    }
                    FRONT_LOG(WARNING) << LOG_DESC("send response message error")
                                       << LOG_KV("routeInfo", printOptionalField(routerInfo))
                                       << LOG_KV("code", error->errorCode())
                                       << LOG_KV("msg", error->errorMessage());
                });
        });
}

void FrontImpl::asyncSendMessageToGateway(bool responsePacket, MessagePayload::Ptr&& frontMessage,
    RouteType routeType, std::string const& traceID, MessageOptionalHeader::Ptr const& routeInfo,
    long timeout, ReceiveMsgFunc callback)
{
    if (responsePacket)
    {
        frontMessage->setRespPacket();
    }
    routeInfo->setSrcNode(m_nodeID);
    auto payload = std::make_shared<bcos::bytes>();
    frontMessage->encode(*payload);
    FRONT_LOG(TRACE) << LOG_DESC("asyncSendMessageToGateway") << LOG_KV("routeType", routeType)
                     << LOG_KV("response", responsePacket) << LOG_KV("traceID", traceID)
                     << printOptionalField(routeInfo)
                     << LOG_KV("payloadSize", frontMessage->length());
    m_gatewayClient->asyncSendMessage(
        routeType, routeInfo, traceID, std::move(*payload), timeout, callback);
}


/**
 * @brief: receive message from gateway, call by gateway
 * @param _message: received ppc message
 * @return void
 */
void FrontImpl::onReceiveMessage(Message::Ptr const& msg, ReceiveMsgFunc callback)
{
    try
    {
        // response to the gateway
        if (callback)
        {
            m_threadPool->enqueue([callback] { callback(nullptr); });
        }
        FRONT_LOG(TRACE) << LOG_BADGE("onReceiveMessage") << LOG_KV("msg", printMessage(msg));
        auto frontMessage = m_messageFactory->build(bcos::ref(*(msg->payload())));
        msg->setFrontMessage(frontMessage);
        // the response packet, dispatcher by callback
        if (frontMessage->isRespPacket())
        {
            handleCallback(nullptr, msg->header()->traceID(), msg);
            return;
        }
        // dispatcher by topic
        m_callbackManager->onReceiveMessage(msg->header()->optionalField()->topic(), msg);
    }
    catch (Exception const& e)
    {
        FRONT_LOG(WARNING) << LOG_DESC("onReceiveMessage exception")
                           << LOG_KV("msg", printMessage(msg))
                           << LOG_KV("error", boost::diagnostic_information(e));
    }
}

// the sync interface for asyncSendMessage
bcos::Error::Ptr FrontImpl::push(uint16_t routeType, MessageOptionalHeader::Ptr const& routeInfo,
    bcos::bytesConstRef payload, int seq, long timeout)
{
    auto promise = std::make_shared<std::promise<bcos::Error::Ptr>>();
    asyncSendMessage(
        routeType, routeInfo, payload, seq, timeout,
        [promise](bcos::Error::Ptr error) { promise->set_value(error); }, nullptr);
    return promise->get_future().get();
}

void FrontImpl::asyncGetPeers(GetPeersInfoHandler::Ptr getPeersCallback)
{
    m_gatewayClient->asyncGetPeers(
        [getPeersCallback](bcos::Error::Ptr error, std::string peersInfo) {
            getPeersCallback->onPeersInfo(error, peersInfo);
        });
}

void FrontImpl::registerComponent(std::string const& component)
{
    // Note: the node will report the latest components
    auto ret = m_nodeInfo->addComponent(component);
    FRONT_LOG(INFO) << LOG_DESC("registerComponent") << LOG_KV("component", component)
                    << LOG_KV("insert", ret);
}

void FrontImpl::updateMetaInfo(std::string const& meta)
{
    // Note: the node will report the latest components
    m_nodeInfo->setMeta(meta);
    FRONT_LOG(INFO) << LOG_DESC("updateMetaInfo") << LOG_KV("meta", meta);
}

void FrontImpl::unRegisterComponent(std::string const& component)
{
    // Note: the node will report the latest components
    auto ret = m_nodeInfo->eraseComponent(component);
    FRONT_LOG(INFO) << LOG_DESC("unRegisterComponent") << LOG_KV("component", component)
                    << LOG_KV("erase", ret);
}