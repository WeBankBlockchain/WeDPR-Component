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
 * @file PPCChannel.cpp
 * @author: shawnhe
 * @date 2022-10-27
 */

#include "PPCChannel.h"

using namespace bcos;
using namespace ppc::front;
using namespace ppc::protocol;


/**
 * @brief: receive message, note that one message will consume one handler
 * @param _messageType: the message type is defined by each crypto algorithm
 * @param _seq: message seq
 * @param _secondsTimeout: timeout by seconds
 * @return void
 */
void PPCChannel::asyncReceiveMessage(uint8_t _messageType, uint32_t _seq, uint32_t _secondsTimeout,
    std::function<void(bcos::Error::Ptr, PPCMessageFace::Ptr)> _handler)
{
    if (!_handler)
    {
        return;
    }

    WriteGuard lock(x_handler_message);

    auto message = getAndRemoveMessage(messageKey(_messageType, _seq));
    if (message)
    {
        // trigger handler by message
        handleCallback(nullptr, message, std::move(_handler));
    }
    else
    {
        auto messageHandler = std::make_shared<MessageHandler>();
        messageHandler->handler = std::move(_handler);

        if (_secondsTimeout == 0)
        {
            _secondsTimeout = HOLDING_MESSAGE_TIMEOUT_M * 60;
        }

        // create timer to handle timeout
        messageHandler->timer = std::make_shared<boost::asio::deadline_timer>(
            *m_ioService, boost::posix_time::seconds(_secondsTimeout));

        messageHandler->timer->async_wait(
            [self = weak_from_this(), _messageType, _seq](boost::system::error_code _error) {
                if (!_error)
                {
                    auto channel = self.lock();
                    if (channel)
                    {
                        // remove timeout handler
                        auto timeoutHandler =
                            channel->getAndRemoveHandler(channel->messageKey(_messageType, _seq));

                        if (timeoutHandler)
                        {
                            FRONT_LOG(DEBUG) << LOG_BADGE("channelWaitingMsgTimeout")
                                             << LOG_KV("messageType", (int)_messageType);

                            // trigger handler by error
                            auto error = std::make_shared<Error>(
                                PPCRetCode::TIMEOUT, "timeout waiting for message");
                            auto handler = timeoutHandler->handler;
                            channel->handleCallback(error, nullptr, std::move(handler));
                        }
                    }
                }
            });

        addHandler(messageKey(_messageType, _seq), std::move(messageHandler));
    }
}


/**
 * @brief: used for front to dispatch message
 * @param _messageType: the message type is defined by each crypto algorithm
 * @return void
 */
void PPCChannel::onMessageArrived(uint8_t _messageType, PPCMessageFace::Ptr _message)
{
    WriteGuard lock(x_handler_message);

    auto seq = _message->seq();
    auto messageHandler = getAndRemoveHandler(messageKey(_messageType, seq));
    if (messageHandler)
    {
        // clear timer
        if (messageHandler->timer)
        {
            messageHandler->timer->cancel();
        }

        // trigger event by buffer
        handleCallback(nullptr, std::move(_message), std::move(messageHandler->handler));
    }
    else
    {
        addMessage(messageKey(_messageType, seq), std::move(_message));
    }
}


void PPCChannel::addHandler(uint64_t _messageKey, MessageHandler::Ptr _handler)
{
    WriteGuard lock(x_handlers);
    m_handlers[_messageKey] = std::move(_handler);
}


PPCChannel::MessageHandler::Ptr PPCChannel::getAndRemoveHandler(uint64_t _messageKey)
{
    WriteGuard lock(x_handlers);
    auto it = m_handlers.find(_messageKey);
    if (it == m_handlers.end())
    {
        return nullptr;
    }

    auto ret = std::move(it->second);
    m_handlers.erase(_messageKey);
    return ret;
}


void PPCChannel::addMessage(uint64_t _messageKey, PPCMessageFace::Ptr _message)
{
    m_messages[_messageKey] = std::move(_message);
}


PPCMessageFace::Ptr PPCChannel::getAndRemoveMessage(uint64_t _messageKey)
{
    auto it = m_messages.find(_messageKey);
    if (it == m_messages.end())
    {
        return nullptr;
    }

    auto ret = std::move(it->second);
    m_messages.erase(_messageKey);
    return ret;
}


void PPCChannel::handleCallback(bcos::Error::Ptr _error, PPCMessageFace::Ptr _message,
    std::function<void(bcos::Error::Ptr, PPCMessageFace::Ptr)>&& _callback)
{
    if (!_callback)
    {
        return;
    }

    if (m_threadPool)
    {
        m_threadPool->enqueue([_error, _message, _callback]() { _callback(_error, _message); });
    }
    else
    {
        _callback(std::move(_error), std::move(_message));
    }
}
