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
 * @file CallbackManager.h
 * @author: yujiechen
 * @date 2024-08-30
 */
#pragma once
#include "ppc-framework/protocol/Message.h"
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/ThreadPool.h>
#include <tbb/concurrent_unordered_map.h>
#include <boost/asio/deadline_timer.hpp>
#include <memory>
#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1
#include <queue>
#include <unordered_map>

namespace ppc::front
{
class Callback
{
public:
    using Ptr = std::shared_ptr<Callback>;
    Callback(ppc::protocol::MessageCallback callback) : msgCallback(std::move(callback)) {}

    ppc::protocol::MessageCallback msgCallback;
    std::shared_ptr<boost::asio::deadline_timer> timeoutHandler;
};
class CallbackManager : public std::enable_shared_from_this<CallbackManager>
{
public:
    using MsgQueueType = bcos::ConcurrentQueue<ppc::protocol::Message::Ptr>;
    using Ptr = std::shared_ptr<CallbackManager>;
    CallbackManager(
        bcos::ThreadPool::Ptr threadPool, std::shared_ptr<boost::asio::io_service> ioService)
      : m_threadPool(std::move(threadPool)), m_ioService(std::move(ioService))
    {}
    virtual ~CallbackManager() = default;

    virtual void addCallback(
        std::string const& traceID, long timeout, ppc::protocol::MessageCallback msgCallback);

    virtual Callback::Ptr pop(std::string const& traceID);

    virtual void handleCallback(bcos::Error::Ptr const& error, std::string const& traceID,
        ppc::protocol::Message::Ptr message, ppc::protocol::SendResponseFunction resFunc);

    virtual void onReceiveMessage(std::string const& topic, ppc::protocol::Message::Ptr msg);

    virtual void registerTopicHandler(
        std::string const& topic, ppc::protocol::MessageDispatcherCallback callback);

    virtual void registerMessageHandler(
        std::string const& componentType, ppc::protocol::MessageDispatcherCallback callback);

    virtual ppc::protocol::Message::Ptr pop(std::string const& topic, int timeoutMs)
    {
        auto it = m_msgCache.find(topic);
        if (it == m_msgCache.end())
        {
            return nullptr;
        }
        auto msgQueue = it->second;
        if (msgQueue->empty())
        {
            return nullptr;
        }
        auto result = msgQueue->tryPop(timeoutMs);
        return result.second;
    }

private:
    void onMessageTimeout(const boost::system::error_code& e, std::string const& traceID);
    void addMsgCache(std::string const& topic, ppc::protocol::Message::Ptr msg)
    {
        auto it = m_msgCache.find(topic);
        if (it == m_msgCache.end())
        {
            m_msgCache.insert(std::make_pair(topic, std::make_shared<MsgQueueType>()));
        }
        auto msgQueue = m_msgCache[topic];
        // push
        msgQueue->push(std::move(msg));
    }

    ppc::protocol::MessageDispatcherCallback getHandlerByTopic(std::string const& topic);
    ppc::protocol::MessageDispatcherCallback getHandlerByComponentType(
        std::string const& componentType);

private:
    bcos::ThreadPool::Ptr m_threadPool;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    // traceID => callback
    std::unordered_map<std::string, Callback::Ptr> m_traceID2Callback;
    mutable bcos::SharedMutex x_traceID2Callback;

    // topic => messageDispatcherCallback
    std::map<std::string, ppc::protocol::MessageDispatcherCallback> m_topicHandlers;
    mutable bcos::SharedMutex x_topicHandlers;

    // componentType => messageDispatcherCallback
    std::map<std::string, ppc::protocol::MessageDispatcherCallback> m_msgHandlers;
    mutable bcos::SharedMutex x_msgHandlers;

    // the messageCache for the message with no topic handler defined
    uint64_t m_maxMsgCacheSize = 10000;
    // TODO: check the queueSize
    tbb::concurrent_unordered_map<std::string, std::shared_ptr<MsgQueueType>> m_msgCache;
};
}  // namespace ppc::front