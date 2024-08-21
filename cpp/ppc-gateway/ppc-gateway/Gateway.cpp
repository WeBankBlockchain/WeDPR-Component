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
 * @file Gateway.cpp
 * @author: shawnhe
 * @date 2022-10-20
 */

#include "Gateway.h"
#include "ProTaskManager.h"
#include <bcos-boostssl/websocket/WsError.h>
#include <bcos-boostssl/websocket/WsMessage.h>
#include <boost/algorithm/string/split.hpp>

using namespace bcos;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;
using namespace bcos::boostssl::context;
using namespace ppc;
using namespace ppc::gateway;
using namespace ppc::protocol;
using namespace ppc::front;


void Gateway::start()
{
    if (m_running)
    {
        GATEWAY_LOG(INFO) << LOG_DESC("Gateway already started");
        return;
    }
    m_running = true;
    GATEWAY_LOG(INFO) << LOG_DESC("start the Gateway");
    // register handler when receiving message from other agencies
    if (m_protocol == m_webSocketService->gatewayConfig()
                          ->config()
                          ->gatewayConfig()
                          .networkConfig.PROTOCOL_WEBSOCKET)
    {
        registerWebSocketMsgHandler();
    }
    else if (m_protocol == m_webSocketService->gatewayConfig()
                               ->config()
                               ->gatewayConfig()
                               .networkConfig.PROTOCOL_HTTP)
    {
#if 0
        // TODO: optimize here
        registerUrlMsgHandler();
#endif
    }
    m_webSocketService->start();
    m_gatewayThread = std::make_shared<std::thread>([&] {
        bcos::pthread_setThreadName("gw_io_service");
        while (m_running.load())
        {
            try
            {
                m_ioContext->run();
                m_ioService->run();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (m_running)
                {
                    if (m_ioService->stopped())
                    {
                        m_ioService->restart();
                    }
                    if (m_ioContext->stopped())
                    {
                        m_ioContext->restart();
                    }
                }
            }
            catch (std::exception& e)
            {
                GATEWAY_LOG(WARNING)
                    << LOG_DESC("Exception in Gateway Thread:") << boost::diagnostic_information(e);
            }
        }
        GATEWAY_LOG(INFO) << "Gateway exit";
    });
    GATEWAY_LOG(INFO) << LOG_BADGE("start the gateway end");
}


void Gateway::stop()
{
    if (!m_running)
    {
        GATEWAY_LOG(INFO) << LOG_DESC("Gateway already stopped");
        return;
    }
    m_running = false;
    GATEWAY_LOG(INFO) << LOG_DESC("stop the Gateway");
    if (m_webSocketService)
    {
        m_webSocketService->stop();
    }
    if (m_ioService)
    {
        m_ioService->stop();
    }
    if (m_ioContext)
    {
        m_ioContext->stop();
    }
    // stop the gateway-thread
    if (m_gatewayThread)
    {
        if (m_gatewayThread->get_id() != std::this_thread::get_id())
        {
            m_gatewayThread->join();
        }
        else
        {
            m_gatewayThread->detach();
        }
    }
    GATEWAY_LOG(INFO) << LOG_BADGE("stop the gateway end");
}

#if 0
// TODO: optimize here
void Gateway::registerUrlMsgHandler()
{
    auto _url = m_webSocketService->gatewayConfig()->config()->gatewayConfig().networkConfig.url;
    m_webSocketService->httpServer()->registerUrlHandler(
        _url, [self = weak_from_this()](bcos::boostssl::http::HttpRequest const& _httpReq,
                  ppc::http::RespUrlFunc _handler) {
            auto gateway = self.lock();
            if (!gateway)
            {
                return;
            }
            std::string code = SEND_MESSAGE_TO_FRONT_SUCCESS_CODE;
            std::string _resMsg = SEND_MESSAGE_TO_FRONT_SUCCESS;
            try
            {
                // handle message
                auto senderStr = _httpReq[HEAD_SENDER_ID].to_string();
                auto taskIdStr = _httpReq[HEAD_TASK_ID].to_string();
                auto algoTypeStr = _httpReq[HEAD_ALGO_TYPE].to_string();
                auto taskTypeStr = _httpReq[HEAD_TASK_TYPE].to_string();
                auto messageTypeStr = _httpReq[HEAD_MESSAGE_TYPE].to_string();
                auto seqStr = _httpReq[HEAD_SEQ].to_string();
                auto uuidStr = _httpReq[HEAD_UUID].to_string();
                bool isResponse = _httpReq[HEAD_IS_RESPONSE].to_string() == "1";
                auto reqBodyBytes = bcos::bytes(_httpReq.body().begin(), _httpReq.body().end());

                GATEWAY_LOG(TRACE) << LOG_BADGE("registerUrlMsgHandler")
                                  << LOG_KV("req size: ", reqBodyBytes.size())
                                  << LOG_KV("taskId: ", taskIdStr)
                                  << LOG_KV("algorithmType: ", algoTypeStr)
                                  << LOG_KV("taskType: ", taskTypeStr)
                                  << LOG_KV("messageType: ", messageTypeStr)
                                  << LOG_KV("UUID: ", uuidStr) << LOG_KV("seq: ", seqStr)
                                  << LOG_KV("isResponse: ", isResponse);

                auto message = gateway->messageFactory()->buildPPCMessage();
                message->setData(std::make_shared<bcos::bytes>(reqBodyBytes));
                message->setTaskID(taskIdStr);
#ifdef ENABLE_CONN
                message->setAlgorithmType(5);
                message->setTaskType(0);
#else
                message->setAlgorithmType(0x00 + atoi(algoTypeStr.c_str()));
                message->setTaskType(0x00 + atoi(taskTypeStr.c_str()));
#endif
                message->setMessageType(0x00 + atoi(messageTypeStr.c_str()));
                message->setSeq(0x00 + atoi(seqStr.c_str()));
                message->setSender(senderStr);
                message->setUuid(uuidStr);
                if (isResponse)
                {
                    message->setResponse();
                }
                gateway->onMessageArrived(std::move(message));
            }
            catch (std::exception& e)
            {
                GATEWAY_LOG(ERROR) << LOG_BADGE("onReceiveMsgFromOtherGateway")
                                   << LOG_KV("exception", boost::diagnostic_information(e));
                code = SEND_MESSAGE_TO_FRONT_ERROR_CODE;
                _resMsg = SEND_MESSAGE_TO_FRONT_ERROR;
            }

            org::interconnection::link::TransportOutbound transportOutbound;
            transportOutbound.set_allocated_message(new std::string(_resMsg));
            transportOutbound.set_allocated_code(new std::string(code));
            std::string responseStr;
            transportOutbound.SerializeToString(&responseStr);
            _handler(nullptr, std::move(bcos::bytes(responseStr.begin(), responseStr.end())));
        });
}
#endif

void Gateway::registerWebSocketMsgHandler()
{
    GATEWAY_LOG(INFO) << LOG_BADGE("registerWebSocketMsgHandler");
    m_webSocketService->webSocketServer()->registerMsgHandler(
        0, [self = weak_from_this()](const std::shared_ptr<MessageFace>& _wsMessage,
               const std::shared_ptr<boostssl::ws::WsSession>& _session) {
            auto gateway = self.lock();
            if (!gateway)
            {
                return;
            }
            try
            {
                // prepare ack
                auto ack = gateway->wsMessageFactory()->buildMessage();
                ack->setRespPacket();
                ack->setPacketType(_wsMessage->packetType());
                ack->setSeq(_wsMessage->seq());
                gateway->addAckCallback(_wsMessage->seq(), ack, _session);

                // handle message
                auto payload = _wsMessage->payload();
                auto message = gateway->messageFactory()->buildPPCMessage(payload);
                if (!message)
                {
                    GATEWAY_LOG(ERROR) << LOG_BADGE("onReceiveMsgFromOtherGateway")
                                       << LOG_DESC("decode ppc message error");
                    gateway->sendAck(_wsMessage->seq(), SEND_MESSAGE_TO_FRONT_ERROR);
                    return;
                }

                GATEWAY_LOG(TRACE)
                    << LOG_BADGE("onReceiveMsgFromOtherGateway")
                    << LOG_KV("taskType", unsigned(message->taskType()))
                    << LOG_KV("algorithmType", unsigned(message->algorithmType()))
                    << LOG_KV("messageType", unsigned(message->messageType()))
                    << LOG_KV("seq", message->seq()) << LOG_KV("taskID", message->taskID())
                    << LOG_KV("sender", message->sender());

                gateway->onMessageArrived(std::move(message));
            }
            catch (std::exception& e)
            {
                GATEWAY_LOG(ERROR) << LOG_BADGE("onReceiveMsgFromOtherGateway")
                                   << LOG_KV("exception", boost::diagnostic_information(e));
                gateway->sendAck(_wsMessage->seq(), SEND_MESSAGE_TO_FRONT_ERROR);
            }
        });
}


/**
 * @brief: send message to other agency
 * @param _agencyID: agency ID of receiver
 * @param _message: ppc message data
 * @return void
 */
void Gateway::asyncSendMessage(
    const std::string& _agencyID, front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback)
{
    auto taskID = _message->taskID();
    GATEWAY_LOG(TRACE) << LOG_BADGE("asyncSendMessage") << printPPCMsg(_message);
    try
    {
        if (m_protocol == m_webSocketService->gatewayConfig()
                              ->config()
                              ->gatewayConfig()
                              .networkConfig.PROTOCOL_WEBSOCKET)
        {
            auto wsClient = m_webSocketService->webSocketClient(_agencyID);
            if (!wsClient)
            {
                onError("asyncSendMessage", taskID,
                    BCOS_ERROR_PTR((int)PPCRetCode::NETWORK_ERROR,
                        "WebSocket client not found for " + _agencyID),
                    _callback);
                return;
            }

            auto payload = std::make_shared<bytes>();
            _message->encode(*payload);
            auto wsMessage = m_wsMessageFactory->buildMessage(0, payload);
            wsMessage->setSeq(_message->uuid());

            // forward to other agency
            auto self = weak_from_this();
            wsClient->asyncSendMessage(wsMessage, Options(m_holdingMessageMinutes * 60 * 1000),
                [self, wsMessage, wsClient, taskID, _callback](const Error::Ptr& _error,
                    const std::shared_ptr<boostssl::MessageFace>& _msg,
                    const std::shared_ptr<boostssl::ws::WsSession>& _session) {
                    Error::Ptr error;
                    // send success
                    if (!_error || _error->errorCode() == 0)
                    {
                        // check ack
                        auto payload = _msg->payload();
                        if (payload)
                        {
                            std::string status = std::string(payload->begin(), payload->end());
                            if (SEND_MESSAGE_TO_FRONT_ERROR == status ||
                                SEND_MESSAGE_TO_FRONT_TIMEOUT == status)
                            {
                                error = std::make_shared<Error>(PPCRetCode::NETWORK_ERROR,
                                    "send message to target front error, status = " + status);
                            }
                        }
                        if (!error)
                        {
                            GATEWAY_LOG(TRACE)
                                << LOG_DESC("asyncSendMessage success") << LOG_KV("task", taskID);
                            // response to the client in-case of tars-error
                            _callback(nullptr);
                            return;
                        }
                    }

                    if (!error)
                    {
                        error = _error;
                    }

                    auto gateway = self.lock();
                    if (!gateway)
                    {
                        return;
                    }
                    gateway->onError("asyncSendMessage", taskID, error, _callback);
                });
        }
#if 0
        else if (m_protocol == m_webSocketService->gatewayConfig()
                                   ->config()
                                   ->gatewayConfig()
                                   .networkConfig.PROTOCOL_HTTP)
        {
            Error::Ptr error;
            auto httpClient = m_webSocketService->httpClient(_agencyID);
            auto _url =
                m_webSocketService->gatewayConfig()->config()->gatewayConfig().networkConfig.url;
            auto header = _message->header();
            appendHeader(header, _message);
            auto body = _message->data();
            auto response = httpClient->post(_url, header, *body);
            // parse response to TransportOutbound
            std::string reponseStr(response.begin(), response.end());
            org::interconnection::link::TransportOutbound transportOutbound;
            transportOutbound.ParseFromString(reponseStr);
            if (transportOutbound.code() != SEND_MESSAGE_TO_FRONT_SUCCESS_CODE)
            {
                error = std::make_shared<Error>(PPCRetCode::NETWORK_ERROR,
                    "send message to target front error, code = " + transportOutbound.code());
            }
            if (!error)
            {
                GATEWAY_LOG(TRACE)
                    << LOG_DESC("asyncSendMessage success") << LOG_KV("task", taskID);
                _callback(nullptr);
                return;
            }
            onError("asyncSendMessage", taskID, error, _callback);
        }
#endif
    }
    catch (std::exception& e)
    {
        onError("asyncSendMessage", taskID,
            BCOS_ERROR_PTR(
                (int)PPCRetCode::EXCEPTION, std::string(boost::diagnostic_information(e))),
            std::move(_callback));
        return;
    }
}

void Gateway::onError(std::string const& _desc, std::string const& _taskID, bcos::Error::Ptr _error,
    ErrorCallbackFunc _callback)
{
    if (!_error || _error->errorCode() == 0)
    {
        return;
    }

    if (_error->errorCode() == WsError::TimeOut)
    {
        // Lower the log level because the caller will type out the error message
        GATEWAY_LOG(INFO) << LOG_BADGE(_desc) << LOG_KV("taskID", _taskID)
                          << LOG_KV("code", _error->errorCode())
                          << LOG_KV("msg", _error->errorMessage());
    }
    else
    {
        GATEWAY_LOG(ERROR) << LOG_BADGE(_desc) << LOG_KV("taskID", _taskID)
                           << LOG_KV("code", _error->errorCode())
                           << LOG_KV("msg", _error->errorMessage());
    }

    if (!_callback)
    {
        return;
    }

    if (m_threadPool)
    {
        m_threadPool->enqueue([_callback, _error]() { _callback(_error); });
    }
    else
    {
        _callback(std::move(_error));
    }
}


/**
 * @brief notice task info to gateway
 * @param _taskInfo the latest task information
 */
bcos::Error::Ptr Gateway::notifyTaskInfo(GatewayTaskInfo::Ptr _taskInfo)
{
    auto startT = bcos::utcSteadyTime();
    auto error = std::make_shared<Error>();
    auto taskID = _taskInfo->taskID;
    auto serviceEndpoint = _taskInfo->serviceEndpoint;
    try
    {
        m_taskManager->registerTaskInfo(taskID, serviceEndpoint);
        // check to see if any message has arrived
        handleHoldingMessageQueue(std::move(_taskInfo));
    }
    catch (std::exception& e)
    {
        error->setErrorCode(PPCRetCode::EXCEPTION);
        error->setErrorMessage(boost::diagnostic_information(e));
    }

    if (error->errorCode())
    {
        GATEWAY_LOG(ERROR) << LOG_BADGE("notifyTaskInfo") << LOG_KV("taskID", taskID)
                           << LOG_DESC(error->errorMessage());
    }
    GATEWAY_LOG(INFO) << LOG_BADGE("notifyTaskInfo") << LOG_KV("taskID", taskID)
                      << LOG_KV("serviceEndpoint", serviceEndpoint)
                      << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
    return error;
}

bcos::Error::Ptr Gateway::eraseTaskInfo(std::string const& _taskID)
{
    auto startT = bcos::utcSteadyTime();
    try
    {
        // release held message
        getAndRemoveHoldingMessages(_taskID);
        m_taskManager->removeTaskInfo(_taskID);
        GATEWAY_LOG(INFO) << LOG_BADGE("eraseTaskInfo") << LOG_KV("taskID", _taskID)
                          << LOG_KV("timecost", bcos::utcSteadyTime() - startT);
        return nullptr;
    }
    catch (std::exception const& e)
    {
        GATEWAY_LOG(ERROR) << LOG_DESC("eraseTaskInfo error")
                           << LOG_KV("exception", boost::diagnostic_information(e))
                           << LOG_KV("timecost", bcos::utcSteadyTime() - startT);
        return BCOS_ERROR_PTR(
            PPCRetCode::EXCEPTION, "eraseTaskInfo error: " + boost::diagnostic_information(e));
    }
}

// register gateway url for other parties
bcos::Error::Ptr Gateway::registerGateway(
    const std::vector<ppc::protocol::GatewayInfo>& _gatewayList)
{
    try
    {
        for (const auto& gateway : _gatewayList)
        {
            m_webSocketService->registerGatewayUrl(gateway.agencyID, gateway.endpoint);
        }

        return nullptr;
    }
    catch (std::exception const& e)
    {
        GATEWAY_LOG(ERROR) << LOG_DESC("registerGateway error")
                           << LOG_KV("exception", boost::diagnostic_information(e));
        return BCOS_ERROR_PTR(
            PPCRetCode::EXCEPTION, "registerGateway error: " + boost::diagnostic_information(e));
    }
}

void Gateway::asyncGetAgencyList(ppc::front::GetAgencyListCallback _callback)
{
    GATEWAY_LOG(TRACE) << LOG_BADGE("asyncGetAgencyList");
    auto const& agencies = m_webSocketService->gatewayConfig()->config()->gatewayConfig().agencies;
    std::vector<std::string> agencyList;
    for (auto const& it : agencies)
    {
        agencyList.emplace_back(it.first);
    }
    if (!_callback)
    {
        return;
    }
    _callback(nullptr, std::move(agencyList));
}

void Gateway::handleHoldingMessageQueue(protocol::GatewayTaskInfo::Ptr _taskInfo)
{
    HoldingMessageQueue::Ptr queue;
    {
        WriteGuard l(x_holdingMessageQueue);
        auto it = m_holdingMessageQueue.find(_taskInfo->taskID);
        // not find the holding-queue related to the task-info
        if (it == m_holdingMessageQueue.end())
        {
            return;
        }
        queue = it->second;
        // erase the queue
        m_holdingMessageQueue.erase(it);
    }
    // cancel the timer
    if (queue->timer)
    {
        queue->timer->cancel();
    }

    auto frontInterface = m_frontNodeManager->getFront(_taskInfo->serviceEndpoint);
    if (!frontInterface)
    {
        GATEWAY_LOG(WARNING) << LOG_DESC(
            "handleHoldingMessageQueue error for not find the corresponding front");
        return;
    }
    // dispatch the message
    for (auto& msg : queue->messages)
    {
        if (!frontInterface)
        {
            GATEWAY_LOG(WARNING) << LOG_DESC("send message error for the target front not found");
            sendAck(msg->uuid(), SEND_MESSAGE_TO_FRONT_ERROR);
            continue;
        }
        // forward to self node
        frontInterface->onReceiveMessage(msg, [self = weak_from_this(), msg](
                                                  const bcos::Error::Ptr& _error) {
            auto gateway = self.lock();
            if (!gateway)
            {
                return;
            }

            if (_error && _error->errorCode() != 0)
            {
                GATEWAY_LOG(WARNING)
                    << LOG_DESC("handleHoldingMessageQueue: dispatch the message error")
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
                gateway->sendAck(msg->uuid(), SEND_MESSAGE_TO_FRONT_ERROR);
                return;
            }

            gateway->sendAck(msg->uuid(), SEND_MESSAGE_TO_FRONT_SUCCESS);
        });
    }
}

// broadcast the message to all front when the task-id is not specified
void Gateway::broadcastMsgToAllFront(ppc::front::PPCMessageFace::Ptr const& _message)
{
    auto frontList = m_frontNodeManager->getAllFront();
    GATEWAY_LOG(TRACE) << LOG_DESC("broadcastMsgToAllFront")
                       << LOG_KV("frontSize", frontList.size());
    for (auto const& it : frontList)
    {
        auto const& front = it.second;
        auto const& serviceEndpoint = it.first;
        dispatchMessageToFront(front, _message, serviceEndpoint);
    }
}

void Gateway::dispatchMessageToFront(ppc::front::FrontInterface::Ptr const& _front,
    ppc::front::PPCMessageFace::Ptr const& _message, std::string const& _serviceEndpoint)
{
    auto taskID = _message->taskID();
    // dispatch message to the given front
    auto startT = utcSteadyTime();
    _front->onReceiveMessage(_message, [self = weak_from_this(), taskID, _serviceEndpoint, _message,
                                           startT](const bcos::Error::Ptr& _error) {
        auto gateway = self.lock();
        if (!gateway)
        {
            return;
        }

        if (_error && _error->errorCode() != 0)
        {
            GATEWAY_LOG(WARNING) << LOG_DESC("onReceiveMessage: dispatch message to front error")
                                 << printPPCMsg(_message) << LOG_KV("task", taskID)
                                 << LOG_KV("front", _serviceEndpoint)
                                 << LOG_KV("code", _error->errorCode())
                                 << LOG_KV("msg", _error->errorMessage())
                                 << LOG_KV("timecost", (utcSteadyTime() - startT));
            gateway->sendAck(_message->uuid(), SEND_MESSAGE_TO_FRONT_ERROR);
            return;
        }
        GATEWAY_LOG(TRACE) << LOG_DESC("onReceiveMessage success") << printPPCMsg(_message)
                           << LOG_KV("timecost", (utcSteadyTime() - startT));
        gateway->sendAck(_message->uuid(), SEND_MESSAGE_TO_FRONT_SUCCESS);
    });
}

void Gateway::onMessageArrived(PPCMessageFace::Ptr _message)
{
    GATEWAY_LOG(TRACE) << LOG_DESC("onMessageArrived") << printPPCMsg(_message);
    auto taskID = _message->taskID();
    // broadcast the message to all front when the task-id is not specified
    if (taskID.empty())
    {
        broadcastMsgToAllFront(_message);
        return;
    }

    bcos::UpgradableGuard l(x_holdingMessageQueue);
    auto serviceEndpoint = m_taskManager->getServiceEndpoint(taskID);
    if (!serviceEndpoint.empty())
    {
        auto frontInterface = m_frontNodeManager->getFront(serviceEndpoint);
        if (!frontInterface)
        {
            GATEWAY_LOG(WARNING)
                << LOG_DESC(
                       "onMessageArrived: can't find the front to dispatch the receive message")
                << printPPCMsg(_message) << LOG_KV("task", taskID)
                << LOG_KV("frontEndPoint", serviceEndpoint);
            sendAck(_message->uuid(), SEND_MESSAGE_TO_FRONT_ERROR);
            return;
        }
        dispatchMessageToFront(frontInterface, _message, serviceEndpoint);
        return;
    }
    // hold the message
    GATEWAY_LOG(INFO) << LOG_BADGE("holdMessage") << LOG_KV("taskID", taskID);

    bcos::UpgradeGuard ul(l);
    auto it = m_holdingMessageQueue.find(taskID);
    if (it != m_holdingMessageQueue.end())
    {
        it->second->messages.emplace_back(_message);
        return;
    }
    // insert new holding-queue
    auto queue = std::make_shared<HoldingMessageQueue>();
    queue->messages.emplace_back(_message);
    // create timer to handle timeout
    queue->timer = std::make_shared<boost::asio::deadline_timer>(
        *m_ioService, boost::posix_time::minutes(m_holdingMessageMinutes));
    queue->timer->async_wait([self = weak_from_this(), taskID](boost::system::error_code _error) {
        if (!_error)
        {
            auto gateway = self.lock();
            if (gateway)
            {
                // remove timeout message
                auto msgQueue = gateway->getAndRemoveHoldingMessages(taskID);
                gateway->handleTimeoutHoldingMessage(msgQueue);
            }
        }
    });
    m_holdingMessageQueue[taskID] = queue;
}

HoldingMessageQueue::Ptr Gateway::getAndRemoveHoldingMessages(const std::string& _taskID)
{
    WriteGuard lock(x_holdingMessageQueue);
    auto it = m_holdingMessageQueue.find(_taskID);
    if (it == m_holdingMessageQueue.end())
    {
        return nullptr;
    }

    HoldingMessageQueue::Ptr ret = it->second;
    m_holdingMessageQueue.erase(_taskID);
    return ret;
}

void Gateway::handleTimeoutHoldingMessage(HoldingMessageQueue::Ptr _queue)
{
    if (!_queue)
    {
        return;
    }
    // dispatch the ack
    for (auto& msg : _queue->messages)
    {
        sendAck(msg->uuid(), SEND_MESSAGE_TO_FRONT_TIMEOUT);
    }
}

void Gateway::addAckCallback(
    std::string const& _uuid, MessageFace::Ptr _msg, boostssl::ws::WsSession::Ptr _session)
{
    WriteGuard lock(x_ackCallbacks);
    m_ackCallbacks[_uuid] = {std::move(_msg), std::move(_session)};
}


void Gateway::sendAck(std::string const& _uuid, std::string const& _status)
{
    WriteGuard lock(x_ackCallbacks);
    auto it = m_ackCallbacks.find(_uuid);
    if (it == m_ackCallbacks.end())
    {
        return;
    }

    auto payload = std::make_shared<bcos::bytes>(_status.begin(), _status.end());
    auto& msg = it->second.first;
    auto& session = it->second.second;
    msg->setPayload(payload);
    session->asyncSendMessage(msg, Options(), nullptr);

    m_ackCallbacks.erase(it);
}

void Gateway::appendHeader(
    std::map<std::string, std::string>& origin_header, front::PPCMessageFace::Ptr _message)
{
    origin_header["Content-Type"] = "application/octet-stream;charset=utf-8";
    origin_header["has_uri"] = std::to_string(true);
    origin_header[HEAD_TASK_ID] = _message->taskID();
    origin_header[HEAD_ALGO_TYPE] = std::to_string(_message->algorithmType());
    origin_header[HEAD_TASK_TYPE] = std::to_string(_message->taskType());
    origin_header[HEAD_SENDER_ID] = _message->sender();
    origin_header[HEAD_MESSAGE_TYPE] = std::to_string(_message->messageType());
    origin_header[HEAD_SEQ] = std::to_string(_message->seq());
    origin_header[HEAD_IS_RESPONSE] = std::to_string(_message->response());
    origin_header[HEAD_UUID] = _message->uuid();
}

Gateway::Ptr GatewayFactory::buildGateway(NodeArch _arch, ppc::tools::PPCConfig::Ptr _config,
    storage::CacheStorage::Ptr _cache, front::PPCMessageFaceFactory::Ptr _messageFactory,
    std::shared_ptr<bcos::ThreadPool> _threadPool)
{
    auto wsMessageFactory = std::make_shared<bcos::boostssl::ws::WsMessageFactory>();
    auto webSocketServiceFactory = std::make_shared<WebSocketServiceFactory>();
    auto ioService = std::make_shared<boost::asio::io_service>();
    auto ioContext = std::make_shared<boost::asio::io_context>();

    auto webSocketService = webSocketServiceFactory->buildWebSocketService(_config, ioContext);

    TaskManager::Ptr taskManager = nullptr;
    if (_arch == NodeArch::AIR || _cache == nullptr)
    {
        GATEWAY_LOG(INFO) << LOG_BADGE("buildGateway without cache");
        taskManager = std::make_shared<TaskManager>(ioService);
    }
    else
    {
        GATEWAY_LOG(INFO) << LOG_BADGE("buildGateway with cache");
        taskManager = std::make_shared<ProTaskManager>(_cache, ioService);
    }
    auto frontNodeManager = std::make_shared<FrontNodeManager>();

    return std::make_shared<Gateway>(std::move(webSocketService), std::move(wsMessageFactory),
        std::move(ioService), std::move(_messageFactory), std::move(frontNodeManager),
        std::move(taskManager), std::move(_threadPool), _config->holdingMessageMinutes(),
        std::move(ioContext));
}
