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
 * @file MessageCache.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/protocol/P2PMessage.h"
#include "tbb/concurrent_vector.h"
#include <bcos-utilities/Common.h>
#include <boost/asio.hpp>
#include <memory>

namespace ppc::gateway
{
struct MessageInfo
{
    ppc::protocol::P2PMessage::Ptr msg;
    ppc::protocol::ReceiveMsgFunc callback;
};
struct HoldingMessageQueue
{
    using Ptr = std::shared_ptr<HoldingMessageQueue>;
    HoldingMessageQueue() = default;

    tbb::concurrent_vector<MessageInfo> messages;
    std::shared_ptr<boost::asio::deadline_timer> timer;
};

class MessageCache : public std::enable_shared_from_this<MessageCache>
{
public:
    using Ptr = std::shared_ptr<MessageCache>;
    MessageCache(std::shared_ptr<boost::asio::io_service> ioService)
      : m_ioService(std::move(ioService))
    {}
    virtual ~MessageCache() = default;

    void insertCache(std::string const& topic, ppc::protocol::P2PMessage::Ptr const& msg,
        ppc::protocol::ReceiveMsgFunc callback);
    HoldingMessageQueue::Ptr pop(std::string const& topic);

private:
    void onTimeout(HoldingMessageQueue::Ptr const& queue);

private:
    int m_holdingMessageMinutes = 30;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    /**
     * hold the message for the situation that
     * gateway receives message from the other side while the task has not been registered.
     */
    mutable bcos::SharedMutex x_msgCache;
    std::unordered_map<std::string, HoldingMessageQueue::Ptr> m_msgCache;
};
}  // namespace ppc::gateway