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
 * @file Front.cpp
 * @author: shawnhe
 * @date 2022-10-20
 */
#include "Front.h"
#include "FrontImpl.h"
#include "ppc-framework/protocol/Constant.h"
#include "ppc-utilities/Utilities.h"

using namespace ppc;
using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::front;

Front::Front(ppc::front::PPCMessageFaceFactory::Ptr ppcMsgFactory, IFront::Ptr front)
  : m_messageFactory(std::move(ppcMsgFactory)), m_front(std::move(front))
{
    m_fetcher = std::make_shared<bcos::Timer>(10 * 1000, "metaFetcher");
    m_fetcher->registerTimeoutHandler([this]() {
        try
        {
            fetchGatewayMetaInfo();
        }
        catch (std::exception const& e)
        {
            FRONT_LOG(WARNING) << LOG_DESC("fetch the gateway information failed")
                               << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}

void Front::start()
{
    m_front->start();
    m_fetcher->start();
}

void Front::stop()
{
    m_fetcher->stop();
    m_front->stop();
}

void Front::fetchGatewayMetaInfo()
{
    auto self = weak_from_this();
    m_front->asyncGetAgencies(m_front->nodeInfo()->copiedComponents(),
        [self](bcos::Error::Ptr error, std::set<std::string> agencies) {
            auto front = self.lock();
            if (!front)
            {
                return;
            }
            if (error && error->errorCode() != 0)
            {
                FRONT_LOG(WARNING)
                    << LOG_DESC("asyncGetAgencies failed") << LOG_KV("code", error->errorCode())
                    << LOG_KV("msg", error->errorMessage());
                return;
            }
            std::vector agencyList(agencies.begin(), agencies.end());
            bcos::UpgradableGuard l(front->x_agencyList);
            if (front->m_agencyList == agencyList)
            {
                return;
            }
            bcos::UpgradeGuard ul(l);
            front->m_agencyList = agencyList;
            FRONT_LOG(INFO) << LOG_DESC("Update agencies information")
                            << LOG_KV("agencies", printVector(agencyList));
        });
    m_fetcher->restart();
}

/**
 * @brief: send message to other party by gateway
 * @param _agencyID: agency ID of receiver
 * @param _message: ppc message data
 * @param _callback: callback called when the message sent successfully
 * @param _respCallback: callback called when receive the response from peer
 * @return void
 */
void Front::asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
    uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _respCallback)
{
    auto front = std::dynamic_pointer_cast<FrontImpl>(m_front);
    auto routeInfo = front->routerInfoBuilder()->build();
    routeInfo->setDstInst(_agencyID);
    routeInfo->setTopic(_message->taskID());
    auto type = ((uint16_t)_message->taskType() << 8) | _message->algorithmType();
    routeInfo->setComponentType(std::to_string(type));
    bcos::bytes data;
    _message->encode(data);
    auto self = weak_from_this();
    ppc::protocol::MessageCallback msgCallback = nullptr;
    if (_respCallback)
    {
        msgCallback = [self, _agencyID, _respCallback](
                          Error::Ptr error, Message::Ptr msg, SendResponseFunction resFunc) {
            auto front = self.lock();
            if (!front)
            {
                return;
            }
            auto responseCallback = [resFunc](PPCMessageFace::Ptr msg) {
                if (!msg)
                {
                    return;
                }
                std::shared_ptr<bcos::bytes> payload = std::make_shared<bcos::bytes>();
                msg->encode(*payload);
                resFunc(std::move(payload));
            };
            if (msg == nullptr)
            {
                _respCallback(error, _agencyID, nullptr, responseCallback);
            }
            // get the agencyID
            _respCallback(error, msg->header()->optionalField()->srcInst(),
                front->m_messageFactory->decodePPCMessage(msg), responseCallback);
        };
    }
    // ROUTE_THROUGH_TOPIC will hold the topic
    m_front->asyncSendMessage((uint16_t)RouteType::ROUTE_THROUGH_TOPIC, routeInfo,
        bcos::bytesConstRef((bcos::byte*)data.data(), data.size()), _message->seq(), _timeout,
        _callback, msgCallback);
    // release the data
    bcos::bytes().swap(data);
}

// send response when receiving message from given agencyID
void Front::asyncSendResponse(bcos::bytes const& dstNode, std::string const& traceID,
    PPCMessageFace::Ptr message, ErrorCallbackFunc _callback)
{
    bcos::bytes data;
    message->encode(data);
    m_front->asyncSendResponse(bcos::bytesConstRef((bcos::byte*)dstNode.data(), dstNode.size()),
        traceID, bcos::bytesConstRef((bcos::byte*)data.data(), data.size()), 0, _callback);
}

/**
 * @brief notice task info to gateway
 * @param _taskInfo the latest task information
 */
bcos::Error::Ptr Front::notifyTaskInfo(std::string const& taskID)
{
    return m_front->registerTopic(taskID);
}

// erase the task-info when task finished
bcos::Error::Ptr Front::eraseTaskInfo(std::string const& _taskID)
{
    FRONT_LOG(INFO) << LOG_DESC("eraseTaskInfo") << LOG_KV("front", m_front);
    return m_front->unRegisterTopic(_taskID);
}

// register message handler for algorithm
void Front::registerMessageHandler(uint8_t _taskType, uint8_t _algorithmType,
    std::function<void(front::PPCMessageFace::Ptr)> _handler)
{
    uint16_t type = ((uint16_t)_taskType << 8) | _algorithmType;
    auto self = weak_from_this();
    m_front->registerMessageHandler(std::to_string(type), [self, type, _handler](
                                                              ppc::protocol::Message::Ptr msg) {
        auto front = self.lock();
        if (!front)
        {
            return;
        }
        try
        {
            if (msg == nullptr)
            {
                _handler(nullptr);
                return;
            }
            _handler(front->m_messageFactory->decodePPCMessage(msg));
            auto length = msg->length();
            // release the payload of the large packet after the msg decoded
            if (length >= ppc::protocol::LARGE_MSG_THRESHOLD)
            {
                FRONT_LOG(INFO) << LOG_DESC("RecvMsg: Release large payload for node after used")
                                << LOG_KV("msgSize", length);
                // release the payload
                msg->releasePayload();
            }
        }
        catch (std::exception const& e)
        {
            FRONT_LOG(WARNING) << LOG_DESC("Call handler for component failed")
                               << LOG_KV("componentType", type)
                               << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
    m_front->registerComponent(std::to_string(type));
}