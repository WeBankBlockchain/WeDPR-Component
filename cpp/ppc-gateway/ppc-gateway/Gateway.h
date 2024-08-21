/**
 *  Copyright (C) 2022 WeDPR.
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
 * @file Gateway.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#include "Common.h"
#include "FrontNodeManager.h"
#include "GatewayService.h"
#include "TaskManager.h"
#include "WebSocketService.h"
#include "ppc-framework/gateway/GatewayInterface.h"
#include "ppc-protocol/src/PPCMessage.h"
#if 0
//TODO: optimize here
#include "ppc-protocol/src/protobuf/transport.pb.h"
#endif

#include "tbb/concurrent_vector.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/ThreadPool.h>

#include <utility>

namespace ppc::gateway
{
struct HoldingMessageQueue
{
    using Ptr = std::shared_ptr<HoldingMessageQueue>;
    HoldingMessageQueue() = default;

    tbb::concurrent_vector<front::PPCMessageFace::Ptr> messages;
    std::shared_ptr<boost::asio::deadline_timer> timer;
};

class Gateway : public GatewayInterface, public std::enable_shared_from_this<Gateway>
{
public:
    using Ptr = std::shared_ptr<Gateway>;

    Gateway(WebSocketService::Ptr _webSocketService,
        bcos::boostssl::ws::WsMessageFactory::Ptr _wsMessageFactory,
        std::shared_ptr<boost::asio::io_service> _ioService,
        front::PPCMessageFaceFactory::Ptr _messageFactory, FrontNodeManager::Ptr _frontNodeManager,
        TaskManager::Ptr _taskManager, std::shared_ptr<bcos::ThreadPool> _threadPool,
        int _holdingMessageMinutes = 10,
        std::shared_ptr<boost::asio::io_context> _ioContext = nullptr)
      : m_holdingMessageMinutes(_holdingMessageMinutes),
        m_webSocketService(std::move(_webSocketService)),
        m_wsMessageFactory(std::move(_wsMessageFactory)),
        m_ioService(std::move(_ioService)),
        m_messageFactory(std::move(_messageFactory)),
        m_frontNodeManager(std::move(_frontNodeManager)),
        m_taskManager(std::move(_taskManager)),
        m_threadPool(std::move(_threadPool)),
        m_ioContext(_ioContext),
        m_protocol(
            _webSocketService->gatewayConfig()->config()->gatewayConfig().networkConfig.protocol)
    {
        GATEWAY_LOG(INFO) << LOG_KV("holdingMessageMinutes", m_holdingMessageMinutes);
    }

    Gateway(const Gateway&) = delete;
    Gateway(Gateway&&) = delete;

    Gateway& operator=(const Gateway&) = delete;
    Gateway& operator=(Gateway&&) = delete;

    virtual ~Gateway() override { stop(); }

    void start() override;
    void stop() override;

    void registerWebSocketMsgHandler();

#if 0
        // TODO: optimize here
    void registerUrlMsgHandler();
#endif

    /**
     * @brief: send message to other agency
     * @param _agencyID: agency ID of receiver
     * @param _message: ppc message data
     * @return void
     */
    void asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
        ErrorCallbackFunc _callback) override;

    /**
     * @brief notice task info to gateway
     * @param _taskInfo the latest task information
     */
    bcos::Error::Ptr notifyTaskInfo(protocol::GatewayTaskInfo::Ptr _taskInfo) override;

    // erase the task info
    bcos::Error::Ptr eraseTaskInfo(std::string const& _taskID) override;

    // register gateway url for other parties
    bcos::Error::Ptr registerGateway(
        const std::vector<ppc::protocol::GatewayInfo>& _gatewayList) override;

    // get the agency-list
    void asyncGetAgencyList(ppc::front::GetAgencyListCallback _callback) override;

    FrontNodeManager::Ptr frontNodeManager() { return m_frontNodeManager; }
    TaskManager::Ptr taskManager() { return m_taskManager; }
    void setTaskManager(TaskManager::Ptr _taskManager) { m_taskManager = std::move(_taskManager); }

    WebSocketService::Ptr webSocketService() { return m_webSocketService; }

    front::PPCMessageFaceFactory::Ptr messageFactory() { return m_messageFactory; }
    bcos::boostssl::ws::WsMessageFactory::Ptr wsMessageFactory() { return m_wsMessageFactory; }
    std::shared_ptr<bcos::ThreadPool> threadPool() { return m_threadPool; }

    void addAckCallback(std::string const& _uuid, bcos::boostssl::MessageFace::Ptr _msg,
        bcos::boostssl::ws::WsSession::Ptr _session);

    void sendAck(std::string const& _uuid, std::string const& _status);

    // Note: since the front will periodically register the status, no need to response message to
    // the front
    void registerFront(std::string const& _endPoint, front::FrontInterface::Ptr _front) override
    {
        m_frontNodeManager->registerFront(_endPoint, _front);
    }

    void unregisterFront(std::string const& _endPoint) override
    {
        m_frontNodeManager->unregisterFront(_endPoint);
    }

protected:
    virtual void handleHoldingMessageQueue(protocol::GatewayTaskInfo::Ptr _taskInfo);
    virtual void onMessageArrived(front::PPCMessageFace::Ptr _message);
    virtual HoldingMessageQueue::Ptr getAndRemoveHoldingMessages(const std::string& _taskID);
    virtual void handleTimeoutHoldingMessage(HoldingMessageQueue::Ptr _queue);
    virtual void onError(std::string const& _desc, std::string const& _taskID,
        bcos::Error::Ptr _error, ErrorCallbackFunc _callback);

    void broadcastMsgToAllFront(ppc::front::PPCMessageFace::Ptr const& _message);
    void dispatchMessageToFront(ppc::front::FrontInterface::Ptr const& _front,
        ppc::front::PPCMessageFace::Ptr const& _message, std::string const& _serviceEndpoint);
    void appendHeader(
        std::map<std::string, std::string>& origin_header, front::PPCMessageFace::Ptr _message);

private:
    int m_holdingMessageMinutes = 30;
    int m_protocol;
    WebSocketService::Ptr m_webSocketService;
    bcos::boostssl::ws::WsMessageFactory::Ptr m_wsMessageFactory;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    std::shared_ptr<boost::asio::io_context> m_ioContext;
    front::PPCMessageFaceFactory::Ptr m_messageFactory;

    FrontNodeManager::Ptr m_frontNodeManager;
    TaskManager::Ptr m_taskManager;

    std::shared_ptr<bcos::ThreadPool> m_threadPool;

    // the thread to make ioservice run
    std::shared_ptr<std::thread> m_gatewayThread;

    /**
     * hold the message for the situation that
     * gateway receives message from the other side while the task has not been registered.
     */
    mutable boost::shared_mutex x_holdingMessageQueue;
    std::unordered_map<std::string, HoldingMessageQueue::Ptr> m_holdingMessageQueue;

    std::atomic_bool m_running = {false};

    mutable boost::shared_mutex x_ackCallbacks;
    std::unordered_map<std::string,
        std::pair<bcos::boostssl::MessageFace::Ptr, bcos::boostssl::ws::WsSession::Ptr> >
        m_ackCallbacks;
};


class GatewayFactory
{
public:
    using Ptr = std::shared_ptr<GatewayFactory>;

public:
    GatewayFactory() = default;
    ~GatewayFactory() = default;

    Gateway::Ptr buildGateway(ppc::protocol::NodeArch _arch, ppc::tools::PPCConfig::Ptr _config,
        storage::CacheStorage::Ptr _cache, front::PPCMessageFaceFactory::Ptr _messageFactory,
        std::shared_ptr<bcos::ThreadPool> _threadPool);
};

}  // namespace ppc::gateway