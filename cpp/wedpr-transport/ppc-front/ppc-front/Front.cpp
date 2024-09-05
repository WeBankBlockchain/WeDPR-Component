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

using namespace ppc;
using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::front;
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
    bcos::bytes data;
    _message->encode(data);
    auto self = weak_from_this();
    // ROUTE_THROUGH_TOPIC will hold the topic
    m_front->asyncSendMessage(RouteType::ROUTE_THROUGH_TOPIC, routeInfo, std::move(data),
        _message->seq(), _timeout, _callback,
        [self, _agencyID, _respCallback](
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
        });
}

// send response when receiving message from given agencyID
void Front::asyncSendResponse(const std::string& _agencyID, std::string const& _uuid,
    front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback)
{}

/**
 * @brief notice task info to gateway
 * @param _taskInfo the latest task information
 */
bcos::Error::Ptr Front::notifyTaskInfo(std::string const& taskID)
{
    m_front->registerTopic(taskID);
}

// erase the task-info when task finished
bcos::Error::Ptr Front::eraseTaskInfo(std::string const& _taskID)
{
    m_front->unRegisterTopic(_taskID);
}