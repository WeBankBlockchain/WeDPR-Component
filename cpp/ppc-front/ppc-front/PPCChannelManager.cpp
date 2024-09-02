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
 * @file PPCChannelManager.cpp
 * @author: shawnhe
 * @date 2022-10-27
 */


#include "PPCChannelManager.h"

#include <utility>

using namespace bcos;
using namespace ppc::front;
using namespace ppc::protocol;

void PPCChannelManager::registerMsgHandlerForChannel(uint8_t _taskType, uint8_t _algorithmType)
{
    FRONT_LOG(INFO) << LOG_BADGE("registerMsgHandlerForChannel") << LOG_KV("taskType", _taskType)
                    << LOG_KV("algorithmType", _algorithmType);
    m_front->registerMessageHandler(
        _taskType, _algorithmType, [self = weak_from_this()](PPCMessageFace::Ptr _message) {
            auto channelManager = self.lock();
            if (channelManager)
            {
                channelManager->onMessageArrived(std::move(_message));
            }
        });
}

Channel::Ptr PPCChannelManager::buildChannelForTask(const std::string& _taskID)
{
    FRONT_LOG(INFO) << LOG_BADGE("buildChannelForTask") << LOG_KV("taskID", _taskID);

    WriteGuard lock(x_message_channel);

    // new a channel
    auto channel = std::make_shared<PPCChannel>(m_ioService, m_front, m_threadPool);
    m_channels[_taskID] = channel;

    // check to see if any message has arrived
    auto it = m_holdingMessages.find(_taskID);
    if (it != m_holdingMessages.end())
    {
        auto holdingMessages = it->second;
        if (holdingMessages->timer)
        {
            holdingMessages->timer->cancel();
        }
        for (auto& msg : holdingMessages->messages)
        {
            channel->onMessageArrived(msg->messageType(), msg);
        }

        // remove holding message
        m_holdingMessages.erase(_taskID);
    }

    return std::static_pointer_cast<Channel>(channel);
}


void PPCChannelManager::removeChannelByTask(const std::string& _taskID)
{
    FRONT_LOG(INFO) << LOG_BADGE("removeChannelByTask") << LOG_KV("taskID", _taskID);

    WriteGuard lock(x_message_channel);
    m_channels.erase(_taskID);
}


void PPCChannelManager::onMessageArrived(PPCMessageFace::Ptr _message)
{
    WriteGuard lock(x_message_channel);

    auto taskID = _message->taskID();

    auto itC = m_channels.find(taskID);
    if (itC != m_channels.end())
    {
        itC->second->onMessageArrived(_message->messageType(), _message);
    }
    else
    {
        // hold the message
        auto itM = m_holdingMessages.find(taskID);
        if (itM != m_holdingMessages.end())
        {
            itM->second->messages.emplace_back(_message);
        }
        else
        {
            auto holdingMessages = std::make_shared<HoldingMessage>();
            holdingMessages->messages = std::vector<PPCMessageFace::Ptr>();
            holdingMessages->messages.emplace_back(_message);

            // create timer to handle timeout
            holdingMessages->timer = std::make_shared<boost::asio::deadline_timer>(
                *m_ioService, boost::posix_time::minutes(HOLDING_MESSAGE_TIMEOUT_M));

            holdingMessages->timer->async_wait(
                [self = weak_from_this(), taskID](boost::system::error_code _error) {
                    if (!_error)
                    {
                        auto channelManager = self.lock();
                        if (channelManager)
                        {
                            // remove timeout message
                            channelManager->removeHoldingMessages(taskID);
                        }
                    }
                });

            m_holdingMessages[taskID] = holdingMessages;
        }
    }
}

void PPCChannelManager::removeHoldingMessages(const std::string& _taskID)
{
    WriteGuard lock(x_message_channel);
    m_holdingMessages.erase(_taskID);
}
