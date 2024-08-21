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
 * @file Front.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#include "Common.h"
#include "FrontService.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/gateway/GatewayInterface.h"
#include "ppc-tars-protocol/ppc-tars-protocol/Common.h"
#include "ppc-tars-protocol/ppc-tars-protocol/client/FrontServiceClient.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/ThreadPool.h>


#include <utility>

namespace ppc
{
namespace front
{
struct Callback
{
    using Ptr = std::shared_ptr<Callback>;
    Callback(CallbackFunc _callback) : callback(std::move(_callback)) {}
    std::shared_ptr<boost::asio::deadline_timer> timeoutHandler;
    CallbackFunc callback;
};
class Front : public front::FrontInterface, public std::enable_shared_from_this<Front>
{
public:
    using Ptr = std::shared_ptr<Front>;

    Front(std::shared_ptr<boost::asio::io_service> _ioService, std::string const& _selfAgencyId,
        bcos::ThreadPool::Ptr _threadPool, std::string const& _selfEndPoint = "localhost")
      : m_ioService(std::move(_ioService)),
        m_selfAgencyId(_selfAgencyId),
        m_threadPool(std::move(_threadPool))
    {
        m_selfEndPoint = _selfEndPoint;
    }

    Front(const Front&) = delete;
    Front(Front&&) = delete;

    Front& operator=(const Front&) = delete;
    Front& operator=(Front&&) = delete;

    virtual ~Front() override = default;

    void start() override;
    void stop() override;

    /**
     * @brief: receive message from gateway, call by gateway
     * @param _message: received ppc message
     * @return void
     */
    void onReceiveMessage(
        front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback) override;


    /**
     * @brief: send message to other party by gateway
     * @param _agencyID: agency ID of receiver
     * @param _message: ppc message data
     * @return void
     */
    void asyncSendMessage(const std::string& _agencyID, PPCMessageFace::Ptr _message,
        uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _respCallback) override;

    void asyncSendResponse(const std::string& _agencyID, std::string const& _uuid,
        front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback) override;

    /**
     * @brief notice task info to gateway
     * @param _taskInfo the latest task information
     */
    bcos::Error::Ptr notifyTaskInfo(protocol::GatewayTaskInfo::Ptr _taskInfo) override;
    // erase the task-info when task finished
    bcos::Error::Ptr eraseTaskInfo(std::string const& _taskID) override;

    // get the agencyList from the gateway
    void asyncGetAgencyList(ppc::front::GetAgencyListCallback _callback) override;

    // register message handler for algorithm
    void registerMessageHandler(uint8_t _taskType, uint8_t _algorithmType,
        std::function<void(front::PPCMessageFace::Ptr)> _handler)
    {
        uint16_t type = ((uint16_t)_taskType << 8) | _algorithmType;
        m_handlers[type] = std::move(_handler);
    }

    ppc::gateway::GatewayInterface::Ptr gatewayInterface() { return m_gatewayInterface; }
    void setGatewayInterface(ppc::gateway::GatewayInterface::Ptr _gatewayInterface)
    {
        m_gatewayInterface = std::move(_gatewayInterface);
    }

    std::shared_ptr<bcos::ThreadPool> threadPool() const { return m_threadPool; }
    const std::string& selfAgencyId() const { return m_selfAgencyId; }
    // Note: the selfEndPoint must be setted before start the front
    virtual void setSelfEndPoint(std::string const& _selfEndPoint)
    {
        FRONT_LOG(INFO) << LOG_DESC("setSelfEndPoint: ") << _selfEndPoint;
        m_selfEndPoint = _selfEndPoint;
    }

private:
    void addCallback(std::string const& _uuid, Callback::Ptr _callback)
    {
        bcos::WriteGuard l(x_uuidToCallback);
        m_uuidToCallback[_uuid] = std::move(_callback);
    }

    Callback::Ptr getAndEraseCallback(std::string const& _uuid)
    {
        bcos::UpgradableGuard l(x_uuidToCallback);
        auto it = m_uuidToCallback.find(_uuid);
        if (it != m_uuidToCallback.end())
        {
            auto callback = it->second;
            bcos::UpgradeGuard ul(l);
            m_uuidToCallback.erase(it);
            return callback;
        }
        return nullptr;
    }

    void onMessageTimeout(const boost::system::error_code& e, std::string const& _agencyID,
        std::string const& _taskID, std::string const& _uuid);
    void handleCallback(bcos::Error::Ptr const& _error, std::string const& _uuid,
        PPCMessageFace::Ptr _message, std::string const& _agencyID);
    void sendMessageToGateway(std::string const& _agencyID, PPCMessageFace::Ptr _msg,
        std::string const& _uuid, bool _response, ErrorCallbackFunc _callback);

private:
    std::shared_ptr<boost::asio::io_service> m_ioService;
    std::string m_selfAgencyId;
    bcos::ThreadPool::Ptr m_threadPool;

    // gatewayInterface
    ppc::gateway::GatewayInterface::Ptr m_gatewayInterface;
    std::unordered_map<uint16_t, std::function<void(front::PPCMessageFace::Ptr)>> m_handlers;

    // uuid->callback
    std::unordered_map<std::string, Callback::Ptr> m_uuidToCallback;
    bcos::SharedMutex x_uuidToCallback;

    bool m_running = false;
    // the thread to run ioservice
    std::shared_ptr<std::thread> m_thread;
};

class FrontFactory
{
public:
    using Ptr = std::shared_ptr<FrontFactory>;
    FrontFactory(std::string _selfAgencyId, std::shared_ptr<bcos::ThreadPool> _threadPool)
      : m_selfAgencyId(std::move(_selfAgencyId)), m_threadPool(std::move(_threadPool))
    {}

public:
    Front::Ptr buildFront(std::shared_ptr<boost::asio::io_service> _ioService);

private:
    std::string m_selfAgencyId;
    // thread pool
    std::shared_ptr<bcos::ThreadPool> m_threadPool;
};

}  // namespace front
}  // namespace ppc
