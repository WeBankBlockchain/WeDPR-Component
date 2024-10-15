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
 * @file PPCChannelManager.h
 * @author: shawnhe
 * @date 2022-10-27
 */

#pragma once

#include "PPCChannel.h"
#include <thread>

namespace ppc::front
{
class PPCChannelManager : public std::enable_shared_from_this<PPCChannelManager>
{
public:
    using Ptr = std::shared_ptr<PPCChannelManager>;
    PPCChannelManager(std::shared_ptr<boost::asio::io_service> _ioService, front::Front::Ptr _front)
      : m_ioService(std::move(_ioService)), m_front(std::move(_front)){};

    ~PPCChannelManager()
    {
        std::unordered_map<std::string, HoldingMessage::Ptr>().swap(m_holdingMessages);
        std::unordered_map<std::string, front::Channel::Ptr>().swap(m_channels);
        FRONT_LOG(INFO) << LOG_DESC("the PPCChannelManager destroyed");
    }

    void setThreadPool(std::shared_ptr<bcos::ThreadPool> _threadPool)
    {
        m_threadPool = std::move(_threadPool);
    }

    /**
     * if the algorithm need to use channel, register the handler of the algorithm in this function.
     */
    void registerMsgHandlerForChannel(uint8_t _taskType, uint8_t _algorithmType);

    /**
     *  build a channel for each task
     */
    Channel::Ptr buildChannelForTask(const std::string& _taskID);

    /**
     *  clean up channel at the end of the task
     */
    void removeChannelByTask(const std::string& _taskID);

protected:
    struct HoldingMessage
    {
        using Ptr = std::shared_ptr<HoldingMessage>;
        std::vector<front::PPCMessageFace::Ptr> messages;
        std::shared_ptr<boost::asio::deadline_timer> timer;
    };

private:
    void onMessageArrived(front::PPCMessageFace::Ptr _message);
    void removeHoldingMessages(const std::string& _taskID);


private:
    std::shared_ptr<boost::asio::io_service> m_ioService;
    front::Front::Ptr m_front;
    std::shared_ptr<bcos::ThreadPool> m_threadPool;

    /**
     * hold the message for the situation that
     * one party receives message from the other side while the Channel has not been registered.
     */
    mutable boost::shared_mutex x_message_channel;
    std::unordered_map<std::string, HoldingMessage::Ptr> m_holdingMessages;
    std::unordered_map<std::string, front::Channel::Ptr> m_channels;
};
}  // namespace ppc::front
