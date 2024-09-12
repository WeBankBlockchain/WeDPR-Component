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
 * @file PPCChannel.h
 * @author: shawnhe
 * @date 2022-10-27
 */

#pragma once

#include "Common.h"
#include "Front.h"
#include "ppc-framework/front/Channel.h"
#include <utility>

namespace ppc::front
{
class PPCChannel : public Channel, public std::enable_shared_from_this<PPCChannel>
{
public:
    using Ptr = std::shared_ptr<PPCChannel>;
    PPCChannel(std::shared_ptr<boost::asio::io_service> _ioService, front::Front::Ptr _front,
        std::shared_ptr<bcos::ThreadPool> _threadPool)
      : m_ioService(std::move(_ioService)),
        m_front(std::move(_front)),
        m_threadPool(std::move(_threadPool))
    {}

    ~PPCChannel() override
    {
        std::unordered_map<uint64_t, MessageHandler::Ptr>().swap(m_handlers);
        std::unordered_map<uint64_t, front::PPCMessageFace::Ptr>().swap(m_messages);
        FRONT_LOG(INFO) << LOG_DESC("the PPCChannel destroyed");
    }

public:
    /**
     * @brief notice task info to gateway by front
     * @param _taskInfo the latest task information
     */
    bcos::Error::Ptr notifyTaskInfo(std::string const& taskID) override
    {
        return m_front->notifyTaskInfo(std::move(taskID));
    };

    /**
     * @brief: send message
     * @param _agencyID: message receiver
     * @return void
     */
    void asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
        uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _respCallback) override
    {
        m_front->asyncSendMessage(_agencyID, std::move(_message), _timeout, std::move(_callback),
            std::move(_respCallback));
    }

    /**
     * @brief: receive message, note that one message will consume one handler
     * @param _messageType: the message type is defined by each crypto algorithm
     * @param _seq: message seq
     * @param _secondsTimeout: timeout by seconds
     * @return void
     */
    void asyncReceiveMessage(uint8_t _messageType, uint32_t _seq, uint32_t _secondsTimeout,
        std::function<void(bcos::Error::Ptr, front::PPCMessageFace::Ptr)> _handler) override;


    /**
     * @brief: used for front to dispatch message
     * @param _messageType: the message type is defined by each crypto algorithm
     * @return void
     */
    void onMessageArrived(uint8_t _messageType, front::PPCMessageFace::Ptr _message) override;


    static inline uint64_t messageKey(uint8_t _messageType, uint32_t _seq)
    {
        uint64_t key = ((uint64_t)_messageType << 32) | _seq;
        return key;
    }

protected:
    struct MessageHandler
    {
        using Ptr = std::shared_ptr<MessageHandler>;
        std::function<void(bcos::Error::Ptr, front::PPCMessageFace::Ptr)> handler;
        std::shared_ptr<boost::asio::deadline_timer> timer;
    };

private:
    void addHandler(uint64_t _messageKey, MessageHandler::Ptr _handler);
    MessageHandler::Ptr getAndRemoveHandler(uint64_t _messageKey);

    void addMessage(uint64_t _messageKey, front::PPCMessageFace::Ptr _message);
    front::PPCMessageFace::Ptr getAndRemoveMessage(uint64_t _messageKey);

    void handleCallback(bcos::Error::Ptr _error, front::PPCMessageFace::Ptr _message,
        std::function<void(bcos::Error::Ptr, front::PPCMessageFace::Ptr)>&& _callback);

private:
    std::shared_ptr<boost::asio::io_service> m_ioService;
    Front::Ptr m_front;
    std::shared_ptr<bcos::ThreadPool> m_threadPool;

    mutable boost::shared_mutex x_handlers;
    // key: messageType || seq
    std::unordered_map<uint64_t, MessageHandler::Ptr> m_handlers;
    std::unordered_map<uint64_t, front::PPCMessageFace::Ptr> m_messages;

    // coordinate handler and message
    mutable boost::shared_mutex x_handler_message;
};
}  // namespace ppc::front