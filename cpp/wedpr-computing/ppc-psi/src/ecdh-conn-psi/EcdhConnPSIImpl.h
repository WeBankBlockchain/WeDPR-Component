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
 * @file EcdhConnPSIImpl.h
 * @author: zachma
 * @date 2023-7-18
 */

#pragma once
#include "../psi-framework/TaskGuarder.h"
#include "Common.h"
#include "EcdhConnPSIConfig.h"
#include "EcdhConnTaskState.h"
#include "core/EcdhConnPSIClient.h"
#include "core/EcdhConnPSIServer.h"
#include "ppc-framework/rpc/RpcInterface.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-rpc/src/RpcFactory.h"
#include "ppc-tools/src/common/ConcurrentPool.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Worker.h>

#include <memory>
#include <queue>

namespace ppc::psi
{
class EcdhConnPSIImpl : public std::enable_shared_from_this<EcdhConnPSIImpl>,
                        public ppc::task::TaskFrameworkInterface,
                        public TaskGuarder,
                        public bcos::Worker
{
public:
    using Ptr = std::shared_ptr<EcdhConnPSIImpl>;
    using EcdhConnPSIMsgPool = ppc::tools::ConcurrentPool<uint16_t, PSIConnMessage::Ptr>;
    using EcdhConnPSIMsgPoolPtr = std::shared_ptr<EcdhConnPSIMsgPool>;

    EcdhConnPSIImpl(const EcdhConnPSIConfig::Ptr& _config, unsigned _idleTimeMs = 0);
    virtual ~EcdhConnPSIImpl() = default;

    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _message) override;


    void start() override;
    void stop() override;

    void onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message) override;
    void onSelfError(
        const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer) override;
    void executeWorker() override;

    void handlerPSIReceiveMessage(PSIConnMessage::Ptr _msg);
    void onHandShakeRequestHandler(const std::string& _taskId, const bcos::bytes& _msg);
    void onHandShakeResponseHandler(const std::string& _taskId, const bcos::bytes& _msg);
    void onCipherClientToServer(const std::string& _taskId, const bcos::bytes& _msg);
    void onCipherServerToClient(const std::string& _taskId, const bcos::bytes& _msg);
    void onSecondCipherClientToServer(const std::string& _taskId, const bcos::bytes& _msg);
    void onSecondCipherServerToClient(const std::string& _taskId, const bcos::bytes& _msg);

    void checkAndSetPeerInfo(TaskState::Ptr const& _taskState);

    void addClient(EcdhConnPSIClient::Ptr _client)
    {
        bcos::WriteGuard l(x_clients);
        m_clients[_client->taskID()] = _client;
    }

    EcdhConnPSIClient::Ptr findClient(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_clients);
        auto it = m_clients.find(_taskID);
        if (it != m_clients.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void addServer(EcdhConnPSIServer::Ptr _server)
    {
        bcos::WriteGuard l(x_servers);
        m_servers[_server->taskID()] = _server;
    }

    EcdhConnPSIServer::Ptr findServer(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_servers);
        auto it = m_servers.find(_taskID);
        if (it != m_servers.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void triggleProcess(uint8_t _mainProcess, uint8_t _subProcess)
    {
        bcos::WriteGuard l(x_process);
        m_process = typeProcess(_mainProcess, _subProcess);
    }

    uint16_t process()
    {
        bcos::ReadGuard l(x_process);
        return m_process;
    }

    uint16_t typeProcess(uint8_t _mainProcess, uint8_t _subProcess)
    {
        return (_subProcess << 8) | _mainProcess;
    }

private:
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signal);
        m_signal.wait_for(l, boost::chrono::milliseconds(5));
    }

    void wakeupWorker() { m_signal.notify_all(); }

    const int c_popWaitMs = 5;
    uint16_t m_process;
    mutable bcos::SharedMutex x_process;

    EcdhConnTaskStateFactory::Ptr m_ecdhConnTaskStateFactory;
    EcdhConnPSIConfig::Ptr m_config;
    EcdhConnTaskState::Ptr m_ecdhTaskState;
    EcdhConnPSIMsgPoolPtr m_msgPool;

    boost::condition_variable m_signal;
    boost::mutex x_signal;

    std::unordered_map<std::string, EcdhConnPSIClient::Ptr> m_clients;
    mutable bcos::SharedMutex x_clients;

    std::unordered_map<std::string, EcdhConnPSIServer::Ptr> m_servers;
    mutable bcos::SharedMutex x_servers;
};
}  // namespace ppc::psi