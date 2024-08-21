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
 * @file Front.cpp
 * @author: shawnhe
 * @date 2022-10-20
 */

#include "Front.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <random>
#include <utility>

using namespace bcos;
using namespace ppc::front;
using namespace ppc::protocol;

void Front::start()
{
    if (m_running)
    {
        FRONT_LOG(INFO) << LOG_DESC("Front has already been started");
        return;
    }
    m_running = true;
    FRONT_LOG(INFO) << LOG_DESC("start the Front");
    m_thread = std::make_shared<std::thread>([&] {
        bcos::pthread_setThreadName("front_io_service");
        while (m_running)
        {
            try
            {
                m_ioService->run();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (m_running && m_ioService->stopped())
                {
                    m_ioService->restart();
                }
            }
            catch (std::exception& e)
            {
                FRONT_LOG(WARNING)
                    << LOG_DESC("Exception in Front Thread:") << boost::diagnostic_information(e);
            }
        }
        FRONT_LOG(INFO) << "Front exit";
    });
}


/**
 * @brief: receive message from gateway, call by gateway
 * @param _message: received ppc message
 * @return void
 */
void Front::onReceiveMessage(PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback)
{
    if (_callback)
    {
        if (m_threadPool)
        {
            m_threadPool->enqueue([_callback] { _callback(nullptr); });
        }
        else
        {
            _callback(nullptr);
        }
    }

    FRONT_LOG(TRACE) << LOG_BADGE("onReceiveMessage") << printPPCMsg(_message);
    // response package
    if (_message->response())
    {
        handleCallback(nullptr, _message->uuid(), _message, _message->sender());
        return;
    }

    uint16_t type = ((uint16_t)_message->taskType() << 8) | _message->algorithmType();
    auto it = m_handlers.find(type);
    if (it != m_handlers.end())
    {
        if (m_threadPool)
        {
            auto handler = it->second;
            m_threadPool->enqueue([handler, _message] { handler(_message); });
        }
        else
        {
            it->second(std::move(_message));
        }
    }
    else
    {
        FRONT_LOG(WARNING) << LOG_BADGE("onReceiveMessage") << LOG_DESC("message handler not found")
                           << LOG_KV("taskType", unsigned(_message->taskType()))
                           << LOG_KV("algorithmType", unsigned(_message->algorithmType()))
                           << LOG_KV("messageType", unsigned(_message->messageType()))
                           << LOG_KV("seq", _message->seq()) << LOG_KV("taskID", _message->taskID())
                           << LOG_KV("sender", _message->sender());
    }
}


/**
 * @brief: send message to other party by gateway
 * @param _agencyID: agency ID of receiver
 * @param _message: ppc message data
 * @return void
 */
void Front::asyncSendMessage(const std::string& _agencyID, PPCMessageFace::Ptr _message,
    uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _respCallback)
{
    // generate uuid for the message
    static thread_local auto uuid_gen = boost::uuids::basic_random_generator<std::random_device>();
    std::string uuid = boost::uuids::to_string(uuid_gen());
    _message->setUuid(uuid);
    // call gateway interface to send the message
    _message->setSender(m_selfAgencyId);
    auto taskID = _message->taskID();
    FRONT_LOG(TRACE) << LOG_BADGE("asyncSendMessage")
                     << LOG_KV("taskType", unsigned(_message->taskType()))
                     << LOG_KV("algorithmType", unsigned(_message->algorithmType()))
                     << LOG_KV("messageType", unsigned(_message->messageType()))
                     << LOG_KV("seq", _message->seq()) << LOG_KV("taskID", _message->taskID())
                     << LOG_KV("receiver", _agencyID) << LOG_KV("uud", _message->uuid());
    // timeout logic
    if (_respCallback)
    {
        auto callback = std::make_shared<Callback>(_respCallback);
        addCallback(uuid, callback);
        if (_timeout > 0)
        {
            auto timeoutHandler = std::make_shared<boost::asio::deadline_timer>(
                *m_ioService, boost::posix_time::milliseconds(_timeout));
            callback->timeoutHandler = timeoutHandler;
            auto self = weak_from_this();
            timeoutHandler->async_wait(
                [self, _agencyID, taskID, uuid](const boost::system::error_code& e) {
                    auto front = self.lock();
                    if (front)
                    {
                        front->onMessageTimeout(e, _agencyID, taskID, uuid);
                    }
                });
        }
    }
    auto self = weak_from_this();
    sendMessageToGateway(_agencyID, std::move(_message), uuid, false,
        [self, uuid, _agencyID, taskID, _callback](const bcos::Error::Ptr& _error) {
            auto front = self.lock();
            if (!front)
            {
                return;
            }
            // send message to gateway error, try to handleCallback
            if (_error && (_error->errorCode() != 0))
            {
                front->handleCallback(_error, uuid, nullptr, _agencyID);
            }
            if (_callback)
            {
                _callback(_error);
            }
        });
}

void Front::handleCallback(bcos::Error::Ptr const& _error, std::string const& _uuid,
    PPCMessageFace::Ptr _message, std::string const& _agencyID)
{
    auto callback = getAndEraseCallback(_uuid);
    if (!callback)
    {
        return;
    }
    // cancel the timer
    if (callback->timeoutHandler)
    {
        callback->timeoutHandler->cancel();
    }
    if (!_message)
    {
        return;
    }
    auto self = weak_from_this();
    auto respFunc = [self, _agencyID, _uuid](PPCMessageFace::Ptr _resp) {
        auto front = self.lock();
        if (!front)
        {
            return;
        }
        front->sendMessageToGateway(front->m_selfAgencyId, std::move(_resp), _uuid, true,
            [_agencyID](const bcos::Error::Ptr& _error) {
                if (!_error)
                {
                    return;
                }
                FRONT_LOG(WARNING)
                    << LOG_DESC("asyncSendResponse message error") << LOG_KV("agency", _agencyID)
                    << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage());
            });
    };
    if (m_threadPool)
    {
        m_threadPool->enqueue([_error, callback, _message, _agencyID, respFunc] {
            callback->callback(_error, _agencyID, _message, respFunc);
        });
    }
    else
    {
        callback->callback(_error, _agencyID, std::move(_message), std::move(respFunc));
    }
}

void Front::sendMessageToGateway(std::string const& _agencyID, PPCMessageFace::Ptr _message,
    std::string const& _uuid, bool _response, ErrorCallbackFunc _callback)
{
    _message->setSender(m_selfAgencyId);
    _message->setUuid(_uuid);
    if (_response)
    {
        _message->setResponse();
    }
    m_gatewayInterface->asyncSendMessage(_agencyID, std::move(_message), std::move(_callback));
}

// send response
void Front::asyncSendResponse(const std::string& _agencyID, std::string const& _uuid,
    front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback)
{
    FRONT_LOG(TRACE) << LOG_DESC("asyncSendResponse") << printPPCMsg(_message);
    sendMessageToGateway(_agencyID, std::move(_message), _uuid, true, std::move(_callback));
}

void Front::onMessageTimeout(const boost::system::error_code& e, std::string const& _agencyID,
    std::string const& _taskID, std::string const& _uuid)
{
    // the timer has been canceled
    if (e)
    {
        return;
    }

    try
    {
        auto callback = getAndEraseCallback(_uuid);
        if (!callback)
        {
            return;
        }
        if (callback->timeoutHandler)
        {
            callback->timeoutHandler->cancel();
        }
        auto errorMsg = "send message with uuid=" + _uuid + ", agency = " + _agencyID +
                        ", task = " + _taskID + " timeout";
        auto error = std::make_shared<Error>(PPCRetCode::TIMEOUT, errorMsg);
        if (m_threadPool)
        {
            m_threadPool->enqueue([callback, _agencyID, error]() {
                callback->callback(error, _agencyID, nullptr, nullptr);
            });
        }
        else
        {
            callback->callback(std::move(error), _agencyID, nullptr, nullptr);
        }
        FRONT_LOG(WARNING) << LOG_BADGE("onMessageTimeout") << LOG_KV("uuid", _uuid)
                           << LOG_KV("agency", _agencyID) << LOG_KV("task", _taskID);
    }
    catch (std::exception& e)
    {
        FRONT_LOG(ERROR) << "onMessageTimeout" << LOG_KV("uuid", _uuid)
                         << LOG_KV("error", boost::diagnostic_information(e));
    }
}


/**
 * @brief notice task info to gateway
 * @param _taskInfo the latest task information
 */
bcos::Error::Ptr Front::notifyTaskInfo(GatewayTaskInfo::Ptr _taskInfo)
{
    auto startT = bcos::utcSteadyTime();
    if (_taskInfo->serviceEndpoint.empty())
    {
        _taskInfo->serviceEndpoint = m_selfEndPoint;
    }
    auto ret = m_gatewayInterface->notifyTaskInfo(_taskInfo);
    FRONT_LOG(INFO) << LOG_BADGE("notifyTaskInfo") << LOG_KV("taskID", _taskInfo->taskID)
                    << LOG_KV("serviceEndpoint", _taskInfo->serviceEndpoint)
                    << LOG_KV("timecost", bcos::utcSteadyTime() - startT);
    return ret;
}

// erase the task-info when task finished
bcos::Error::Ptr Front::eraseTaskInfo(std::string const& _taskID)
{
    auto startT = bcos::utcSteadyTime();
    auto ret = m_gatewayInterface->eraseTaskInfo(_taskID);
    FRONT_LOG(INFO) << LOG_BADGE("eraseTaskInfo") << LOG_KV("taskID", _taskID)
                    << LOG_KV("timecost", bcos::utcSteadyTime() - startT);
    return ret;
}

// get the agencyList from the gateway
void Front::asyncGetAgencyList(ppc::front::GetAgencyListCallback _callback)
{
    FRONT_LOG(TRACE) << LOG_BADGE("asyncGetAgencyList");
    if (!m_gatewayInterface)
    {
        std::vector<std::string> emptyAgencies;
        _callback(std::make_shared<bcos::Error>(
                      -1, "asyncGetAgencyList failed for the gateway not been inited into front!"),
            std::move(emptyAgencies));
        return;
    }
    m_gatewayInterface->asyncGetAgencyList(std::move(_callback));
}

void Front::stop()
{
    if (!m_running)
    {
        FRONT_LOG(INFO) << LOG_DESC("Front has already been stopped");
        return;
    }
    m_running = false;
    if (m_ioService)
    {
        m_ioService->stop();
    }
    if (m_thread)
    {
        // stop the thread
        if (m_thread->get_id() != std::this_thread::get_id())
        {
            m_thread->join();
        }
        else
        {
            m_thread->detach();
        }
    }
}


Front::Ptr FrontFactory::buildFront(std::shared_ptr<boost::asio::io_service> _ioService)
{
    FRONT_LOG(INFO) << LOG_BADGE("buildFront") << LOG_KV("agencyID", m_selfAgencyId);
    return std::make_shared<Front>(std::move(_ioService), m_selfAgencyId, m_threadPool);
}