/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file MessageCache.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */

#include "MessageCache.h"
#include "ppc-gateway/Common.h"

using namespace ppc;
using namespace bcos;
using namespace ppc::protocol;
using namespace ppc::gateway;

void MessageCache::insertCache(
    std::string const& topic, ppc::protocol::P2PMessage::Ptr const& msg, ReceiveMsgFunc callback)
{
    // hold the message
    GATEWAY_LOG(DEBUG) << LOG_BADGE("MessageCache: insertCache") << printP2PMessage(msg);
    bcos::ReadGuard l(x_msgCache);
    auto it = m_msgCache.find(topic);
    if (it != m_msgCache.end())
    {
        it->second->messages.emplace_back(MessageInfo{msg, callback});
        return;
    }
    // insert new holding-queue
    auto queue = std::make_shared<HoldingMessageQueue>();
    queue->messages.emplace_back(MessageInfo{msg, callback});
    // create timer to handle timeout
    queue->timer = std::make_shared<boost::asio::deadline_timer>(
        *m_ioService, boost::posix_time::minutes(m_holdingMessageMinutes));
    queue->timer->async_wait([self = weak_from_this(), topic](boost::system::error_code _error) {
        if (!_error)
        {
            auto cache = self.lock();
            if (cache)
            {
                // remove timeout message
                auto msgQueue = cache->pop(topic);
                if (!msgQueue)
                {
                    return;
                }
                msgQueue->timer->cancel();
                cache->onTimeout(msgQueue);
            }
        }
    });
    m_msgCache[topic] = queue;
}

HoldingMessageQueue::Ptr MessageCache::pop(const std::string& topic)
{
    WriteGuard lock(x_msgCache);
    auto it = m_msgCache.find(topic);
    if (it == m_msgCache.end())
    {
        return nullptr;
    }
    HoldingMessageQueue::Ptr ret = it->second;
    m_msgCache.erase(topic);
    return ret;
}

void MessageCache::onTimeout(HoldingMessageQueue::Ptr const& queue)
{
    if (!queue)
    {
        return;
    }
    // dispatch the ack
    for (auto& msgInfo : queue->messages)
    {
        if (msgInfo.callback)
        {
            msgInfo.callback(std::make_shared<bcos::Error>(-1, "timeout"));
        }
    }
}