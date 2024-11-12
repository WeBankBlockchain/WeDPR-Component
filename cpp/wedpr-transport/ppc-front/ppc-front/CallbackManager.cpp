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
 * @file CallbackManager.cpp
 * @author: yujiechen
 * @date 2024-08-30
 */
#include "CallbackManager.h"
#include "Common.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/Common.h>

using namespace bcos;
using namespace ppc;
using namespace ppc::front;
using namespace ppc::protocol;

void CallbackManager::addCallback(
    std::string const& traceID, long timeout, ppc::protocol::MessageCallback msgCallback)
{
    if (!msgCallback)
    {
        return;
    }
    auto callback = std::make_shared<Callback>(msgCallback);
    // set the timeout handler
    if (timeout > 0)
    {
        callback->timeoutHandler = std::make_shared<boost::asio::deadline_timer>(
            *m_ioService, boost::posix_time::milliseconds(timeout));
        auto self = weak_from_this();
        callback->timeoutHandler->async_wait([self, traceID](const boost::system::error_code& e) {
            auto front = self.lock();
            if (front)
            {
                front->onMessageTimeout(e, traceID);
            }
        });
    }
    // insert the callback into m_traceID2Callback
    WriteGuard l(x_traceID2Callback);
    m_traceID2Callback.insert(std::make_pair(traceID, callback));
}

Callback::Ptr CallbackManager::pop(std::string const& traceID)
{
    bcos::UpgradableGuard l(x_traceID2Callback);
    auto it = m_traceID2Callback.find(traceID);
    if (it == m_traceID2Callback.end())
    {
        return nullptr;
    }
    auto callback = it->second;
    m_traceID2Callback.erase(it);
    return callback;
}

void CallbackManager::onMessageTimeout(
    const boost::system::error_code& e, std::string const& traceID)
{
    // the timer has been canceled
    if (e)
    {
        return;
    }
    try
    {
        auto callback = pop(traceID);
        if (!callback)
        {
            return;
        }
        if (callback->timeoutHandler)
        {
            callback->timeoutHandler->cancel();
        }
        auto errorMsg = "send message with traceID=" + traceID + " timeout";
        auto error = std::make_shared<Error>(PPCRetCode::TIMEOUT, errorMsg);
        m_threadPool->enqueue(
            [callback, error]() { callback->msgCallback(error, nullptr, nullptr); });
        FRONT_LOG(WARNING) << LOG_BADGE("onMessageTimeout") << LOG_KV("traceID", traceID);
    }
    catch (std::exception const& e)
    {
        FRONT_LOG(WARNING) << LOG_BADGE("onMessageTimeout") << LOG_KV("traceID", traceID)
                           << LOG_KV("error", boost::diagnostic_information(e));
    }
}


void CallbackManager::handleCallback(bcos::Error::Ptr const& error, std::string const& traceID,
    Message::Ptr message, SendResponseFunction resFunc)
{
    auto callback = pop(traceID);
    if (!callback)
    {
        return;
    }
    if (callback->timeoutHandler)
    {
        callback->timeoutHandler->cancel();
    }
    if (!message)
    {
        return;
    }
    m_threadPool->enqueue([error, callback, message, resFunc] {
        try
        {
            callback->msgCallback(error, message, resFunc);
        }
        catch (std::exception const& e)
        {
            FRONT_LOG(WARNING) << LOG_DESC("handleCallback exception")
                               << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}


void CallbackManager::registerTopicHandler(
    std::string const& topic, ppc::protocol::MessageDispatcherCallback callback)
{
    bcos::WriteGuard l(x_topicHandlers);
    m_topicHandlers.insert(std::make_pair(topic, callback));
}

void CallbackManager::registerMessageHandler(
    std::string const& componentType, ppc::protocol::MessageDispatcherCallback callback)
{
    bcos::WriteGuard l(x_msgHandlers);
    m_msgHandlers.insert(std::make_pair(componentType, callback));
}

MessageDispatcherCallback CallbackManager::getHandlerByTopic(std::string const& topic)
{
    bcos::ReadGuard l(x_topicHandlers);
    auto it = m_topicHandlers.find(topic);
    if (it != m_topicHandlers.end())
    {
        return it->second;
    }
    return nullptr;
}

MessageDispatcherCallback CallbackManager::getHandlerByComponentType(
    std::string const& componentType)
{
    bcos::ReadGuard l(x_msgHandlers);
    auto it = m_msgHandlers.find(componentType);
    if (it != m_msgHandlers.end())
    {
        return it->second;
    }
    return nullptr;
}

void CallbackManager::onReceiveMessage(std::string const& topic, Message::Ptr msg)
{
    auto callback = getHandlerByTopic(topic);
    if (!callback)
    {
        callback = getHandlerByComponentType(msg->header()->optionalField()->componentType());
    }
    if (!callback)
    {
        if (topic.empty())
        {
            return;
        }
        FRONT_LOG(TRACE) << LOG_DESC("onReceiveMessage: not find the handler, put into the buffer")
                         << LOG_KV("topic", topic);
        addMsgCache(topic, msg);
        return;
    }
    m_threadPool->enqueue([callback, msg]() {
        try
        {
            callback(std::move(msg));
        }
        catch (Exception e)
        {
            FRONT_LOG(WARNING) << LOG_DESC("onReceiveMessage: dispatcher exception")
                               << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}