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
 * @brief interface for front service module
 * @file FrontInterface.h
 * @author: shawnhe
 * @date 2022-10-19
 */

#pragma once
#include "../protocol/PPCMessageFace.h"
#include "ppc-framework/protocol/Task.h"
#include <bcos-utilities/Error.h>

namespace ppc
{
namespace front
{
using ErrorCallbackFunc = std::function<void(bcos::Error::Ptr)>;
using ResponseFunc = std::function<void(PPCMessageFace::Ptr)>;
using CallbackFunc = std::function<void(bcos::Error::Ptr _error, std::string const& _agencyID,
    PPCMessageFace::Ptr _message, ResponseFunc _respFunc)>;

using GetAgencyListCallback = std::function<void(bcos::Error::Ptr, std::vector<std::string>&&)>;

/**
 * @brief: the interface provided by the front service
 */
class FrontInterface
{
public:
    using Ptr = std::shared_ptr<FrontInterface>;
    FrontInterface() = default;
    virtual ~FrontInterface() {}
    /**
     * @brief: start/stop service
     */
    virtual void start() = 0;
    virtual void stop() = 0;

    /**
     * @brief: receive message from gateway, call by gateway
     * @param _message: received ppc message
     * @return void
     */
    virtual void onReceiveMessage(
        front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback) = 0;


    /**
     * @brief: send message to other party by gateway
     * @param _agencyID: agency ID of receiver
     * @param _message: ppc message data
     * @param _callback: callback called when the message sent successfully
     * @param _respCallback: callback called when receive the response from peer
     * @return void
     */
    virtual void asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
        uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _respCallback) = 0;

    // send response when receiving message from given agencyID
    virtual void asyncSendResponse(const std::string& _agencyID, std::string const& _uuid,
        front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback) = 0;

    /**
     * @brief notice task info to gateway
     * @param _taskInfo the latest task information
     */
    virtual bcos::Error::Ptr notifyTaskInfo(protocol::GatewayTaskInfo::Ptr _taskInfo) = 0;

    // erase the task-info when task finished
    virtual bcos::Error::Ptr eraseTaskInfo(std::string const& _taskID) = 0;
    
    // get the agencyList from the gateway
    virtual void asyncGetAgencyList(GetAgencyListCallback _callback) = 0;

    virtual std::string const& selfEndPoint() const { return m_selfEndPoint; }

protected:
    // the selfEndPoint for the air-mode-node can be localhost
    std::string m_selfEndPoint = "localhost";
};

}  // namespace front
}  // namespace ppc
