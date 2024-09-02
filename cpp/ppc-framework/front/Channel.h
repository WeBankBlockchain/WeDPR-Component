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
 * @brief interface for channel which is the abstract communication unit of the privacy service
 * @file Channel.h
 * @author: shawnhe
 * @date 2022-10-27
 */

#pragma once
#include "FrontInterface.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include "ppc-framework/protocol/Task.h"
#include <bcos-utilities/Error.h>
#include <memory>

namespace ppc
{
namespace front
{
/**
 * Each task hold a channel instance.
 * The front can dispatch the message to the specific channel by task ID.
 */
class Channel
{
public:
    using Ptr = std::shared_ptr<Channel>;
    Channel() = default;
    virtual ~Channel() {}

public:
    /**
     * @brief notice task info to gateway by front
     * @param _taskInfo the latest task information
     */
    virtual bcos::Error::Ptr notifyTaskInfo(std::string const& taskID) = 0;

    /**
     * @brief: send message
     * @param _agencyID: message receiver
     * @return void
     */
    virtual void asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
        uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _respCallback) = 0;

    /**
     * @brief: receive message, note that one message will consume one handler
     * @param _messageType: the message type is defined by each crypto algorithm
     * @param _seq: message seq
     * @param _secondsTimeout: timeout by seconds
     * @return void
     */
    virtual void asyncReceiveMessage(uint8_t _messageType, uint32_t _seq, uint32_t _secondsTimeout,
        std::function<void(bcos::Error::Ptr, front::PPCMessageFace::Ptr)> _handler) = 0;


    /**
     * @brief: used for front to dispatch message
     * @param _messageType: the message type is defined by each crypto algorithm
     * @return void
     */
    virtual void onMessageArrived(uint8_t _messageType, front::PPCMessageFace::Ptr _message) = 0;
};

}  // namespace front
}  // namespace ppc