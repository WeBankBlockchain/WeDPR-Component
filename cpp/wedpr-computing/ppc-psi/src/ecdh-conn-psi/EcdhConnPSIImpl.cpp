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
 * @file EcdhConnPSIImpl.cpp
 * @author: zachma
 * @date 2023-8-28
 */

#include "EcdhConnPSIImpl.h"
#include <bcos-utilities/DataConvertUtility.h>

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::tools;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace bcos;
using namespace ppc::task;

EcdhConnPSIImpl::EcdhConnPSIImpl(const EcdhConnPSIConfig::Ptr& _config, unsigned _idleTimeMs)
  : m_config(std::move(_config)),
    m_msgPool(std::make_shared<EcdhConnPSIMsgPool>()),
    TaskGuarder(_config, TaskAlgorithmType::ECDH_PSI_CONN, "ECDH-CONN-PSI-Timer"),
    m_ecdhConnTaskStateFactory(std::make_shared<EcdhConnTaskStateFactory>())
{}

void EcdhConnPSIImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, ppc::task::TaskResponseCallback&& _onTaskFinished)
{
    ECDH_CONN_LOG(INFO) << LOG_DESC("Start a Conn asyncRunTask");
    auto ecdhTaskState = m_ecdhConnTaskStateFactory->createConnTaskState(
        _task, std::move(_onTaskFinished), true, m_config);
    auto dataResource = _task->selfParty()->dataResource();
    auto reader = loadReader(_task->id(), dataResource, DataSchema::Bytes);
    ecdhTaskState->setReader(reader, -1);
    auto role = _task->selfParty()->partyIndex();
    checkAndSetPeerInfo(ecdhTaskState);
    // init process
    triggleProcess((uint8_t)EcdhConnProcess::HandShakeProcess, -1);
    // notify the taskInfo to the front
    m_config->front()->notifyTaskInfo(_task->id());
    if (role == uint16_t(PartyType::Client))
    {
        ECDH_CONN_LOG(INFO) << LOG_DESC("Client do the Task") << LOG_KV("taskID", _task->id());
        auto writer = loadWriter(_task->id(), dataResource, m_enableOutputExists);
        ecdhTaskState->setWriter(writer);
        auto client = std::make_shared<EcdhConnPSIClient>(m_config, ecdhTaskState);
        addClient(client);
        client->asyncStartRunTask(_task);
    }
    else if (role == uint16_t(PartyType::Server))
    {
        ECDH_CONN_LOG(INFO) << LOG_DESC("Server do the Task") << LOG_KV("taskID", _task->id());
        if (_task->syncResultToPeer())
        {
            auto writer = loadWriter(_task->id(), dataResource, m_enableOutputExists);
            ecdhTaskState->setWriter(writer);
        }
        auto server = std::make_shared<EcdhConnPSIServer>(m_config, ecdhTaskState);
        addServer(server);
        server->asyncStartRunTask(_task);
    }
    else
    {
        auto taskResult = std::make_shared<TaskResult>(_task->id());
        auto error = BCOS_ERROR_PTR((int)TaskParamsError,
            "The party index of the ecdh-conn-psi must be client(0) or server(1)");
        ECDH_CONN_LOG(WARNING) << LOG_DESC("response task result") << LOG_KV("task", _task)
                               << LOG_KV("code", error->errorCode())
                               << LOG_KV("msg", error->errorMessage());
        taskResult->setError(std::move(error));
        _onTaskFinished(std::move(taskResult));
        return;
    }
}

void EcdhConnPSIImpl::onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg)
{
    try
    {
        auto payLoad = _msg->data();
        auto psiConnMsg = m_config->msgFactory()->decodePSIConnMessage(*payLoad);
        if (psiConnMsg == nullptr)
        {
            return;
        }
        psiConnMsg->setTaskID(_msg->taskID());
        auto type = typeProcess(psiConnMsg->mainProcess(), psiConnMsg->subProcess());
        m_msgPool->insert(type, psiConnMsg);
        wakeupWorker();
    }
    catch (std::exception const& e)
    {
        ECDH_CONN_LOG(WARNING) << LOG_DESC("onReceiveMessage exception") << printPPCMsg(_msg)
                               << LOG_KV("error", boost::diagnostic_information(e));
    }
}


void EcdhConnPSIImpl::start()
{
    startWorking();
}

void EcdhConnPSIImpl::stop()
{
    if (m_config->threadPool())
    {
        m_config->threadPool()->stop();
    }

    finishWorker();
    if (isWorking())
    {
        // stop the worker thread
        stopWorking();
        terminate();
    }
}

void EcdhConnPSIImpl::onReceivedErrorNotification(const std::string& _taskID) {}

void EcdhConnPSIImpl::onSelfError(
    const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer)
{}

void EcdhConnPSIImpl::checkAndSetPeerInfo(TaskState::Ptr const& _taskState)
{
    auto task = _taskState->task();
    // check the peer
    auto peerParties = task->getAllPeerParties();
    if (peerParties.size() == 1)
    {
        auto peerParty = peerParties.begin()->second;
        _taskState->setPeerID(peerParty->id());
        ECDH_CONN_LOG(INFO) << LOG_DESC("checkAndSetPeerInfo success");
    }
}

void EcdhConnPSIImpl::executeWorker()
{
    // auto _msg = m_msgQueue->tryPop(c_popWaitMs);
    auto _msg = m_msgPool->tryPop(process(), c_popWaitMs);
    while (_msg.first && process() != typeProcess((uint8_t)EcdhConnProcess::END, -2))
    {
        try
        {
            handlerPSIReceiveMessage(_msg.second);
            _msg = m_msgPool->tryPop(process(), c_popWaitMs);
        }
        catch (std::exception const& e)
        {
            ECDH_CONN_LOG(WARNING) << LOG_DESC("executeWorker exception")
                                   << LOG_KV("error", boost::diagnostic_information(e));
        }
        waitSignal();
    }
}

void EcdhConnPSIImpl::handlerPSIReceiveMessage(PSIConnMessage::Ptr _msg)
{
    auto self = weak_from_this();
    m_config->threadPool()->enqueue([self, _msg]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        try
        {
            auto mainProcess = _msg->mainProcess();
            auto subProcess = _msg->subProcess();
            auto receiver = _msg->receiver();
            auto _taskId = _msg->taskID();
            if (mainProcess == int(EcdhConnProcess::HandShakeProcess) && subProcess == uint8_t(-1))
            {
                if (receiver == int(PartyType::Client))  // PART2 : server -> client handshake
                {
                    ECDH_CONN_LOG(INFO) << LOG_DESC(" PART2 : server -> client handshake ")
                                        << LOG_KV("PartyType::Client ", _msg->value().data());
                    psi->onHandShakeResponseHandler(_taskId, _msg->value());
                }
                else if (receiver == int(PartyType::Server))  // PART1 : client -> server handshake
                {
                    ECDH_CONN_LOG(INFO) << LOG_DESC(" PART1 : client -> server handshake ")
                                        << LOG_KV("PartyType::Server ", _msg->value().data());
                    psi->onHandShakeRequestHandler(_taskId, _msg->value());
                }
                psi->triggleProcess((uint8_t)EcdhConnProcess::CipherProcess, -1);
            }
            else if (mainProcess == int(EcdhConnProcess::CipherProcess) &&
                     subProcess == uint8_t(-1))
            {
                if (receiver == int(PartyType::Client))  // PART3 : server -> client cipher
                {
                    ECDH_CONN_LOG(INFO) << LOG_DESC(" PART3 : server -> client cipher ")
                                        << LOG_KV("PartyType::Client ", _msg->value().data());
                    psi->onCipherServerToClient(_taskId, _msg->value());
                }
                else if (receiver == int(PartyType::Server))  // PART4 : client -> server cipher
                {
                    ECDH_CONN_LOG(INFO) << LOG_DESC(" PART4 : client -> server cipher ")
                                        << LOG_KV("PartyType::Server ", _msg->value().data());
                    psi->onCipherClientToServer(_taskId, _msg->value());
                }
                psi->triggleProcess((uint8_t)EcdhConnSubProcess::CipherSecondProcess, 0);
            }
            else if (mainProcess == int(EcdhConnSubProcess::CipherSecondProcess) &&
                     subProcess == uint8_t(0))
            {
                if (receiver == int(PartyType::Client))  // PART6 : server -> client cipher second
                {
                    ECDH_CONN_LOG(INFO)
                        << LOG_DESC(" PART5 : server -> client cipher second ")
                        << LOG_KV("PartyType::Client ", *toHexString(_msg->value()));
                    psi->onSecondCipherServerToClient(_taskId, _msg->value());
                }
                else if (receiver == int(PartyType::Server))  // PART5 : client -> server cipher
                                                              // second
                {
                    ECDH_CONN_LOG(INFO)
                        << LOG_DESC(" PART6 : client -> server cipher second ")
                        << LOG_KV("PartyType::Server ", *toHexString(_msg->value()));
                    psi->onSecondCipherClientToServer(_taskId, _msg->value());
                }
                psi->triggleProcess((uint8_t)EcdhConnProcess::END, -2);
            }
        }
        catch (std::exception const& e)
        {
            ECDH_CONN_LOG(WARNING)
                << LOG_DESC("handlePSIMsg exception") << LOG_KV("mainProcess", _msg->mainProcess())
                << printPSIConnMessage(_msg) << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}

// Http PART1: client -> server : handshake request
void EcdhConnPSIImpl::onHandShakeRequestHandler(const std::string& _taskId, const bcos::bytes& _msg)
{
    auto server = findServer(_taskId);
    if (server)
    {
        server->onHandShakeRequestHandler(_msg);
    }
}

// Http PART2: server -> client : handshake response
void EcdhConnPSIImpl::onHandShakeResponseHandler(
    const std::string& _taskId, const bcos::bytes& _msg)
{
    auto client = findClient(_taskId);
    if (client)
    {
        client->onHandShakeResponseHandler(_msg);
    }
}

// Http Part3: client -> server : cipher
void EcdhConnPSIImpl::onCipherClientToServer(const std::string& _taskId, const bcos::bytes& _msg)
{
    auto server = findServer(_taskId);
    if (server)
    {
        server->onCipherClientToServer(_msg);
    }
}

// Http Part4: server -> client : cipher
void EcdhConnPSIImpl::onCipherServerToClient(const std::string& _taskId, const bcos::bytes& _msg)
{
    auto client = findClient(_taskId);
    if (client)
    {
        client->onCipherServerToClient(_msg);
    }
}

// Http Part5: client -> server : cipher second
void EcdhConnPSIImpl::onSecondCipherClientToServer(
    const std::string& _taskId, const bcos::bytes& _msg)
{
    auto server = findServer(_taskId);
    if (server)
    {
        server->onSecondCipherClientToServer(_msg);
    }
}

// Http Part6: server -> client : cipher second
void EcdhConnPSIImpl::onSecondCipherServerToClient(
    const std::string& _taskId, const bcos::bytes& _msg)
{
    auto client = findClient(_taskId);
    if (client)
    {
        client->onSecondCipherServerToClient(_msg);
    }
}
