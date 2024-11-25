/*
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
 * @file Rpc.cpp
 * @author: yujiechen
 * @date 2022-11-3
 */
#include "Rpc.h"
#include "JsonRequest.h"
#include "JsonResponse.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-framework/rpc/RpcTypeDef.h"
#include "ppc-tools/src/config/ParamChecker.h"
#include <bcos-utilities/Error.h>

using namespace bcos;
using namespace ppc::rpc;
using namespace ppc::tools;
using namespace ppc::protocol;

Rpc::Rpc(std::shared_ptr<boostssl::ws::WsService> _wsService, ppc::gateway::IGateway::Ptr gateway,
    std::string const& _selfPartyID, std::string const& _token, std::string const& _prePath)
  : m_prePath(_prePath),
    m_wsService(std::move(_wsService)),
    m_gateway(std::move(gateway)),
    m_taskFactory(std::make_shared<JsonTaskFactory>(_selfPartyID, _prePath)),
    m_token(_token)
{
    // register handler for wsService
    m_wsService->registerMsgHandler((uint16_t)ppc::protocol::MessageType::RpcRequest,
        boost::bind(&Rpc::onWsRpcRequest, this, boost::placeholders::_1, boost::placeholders::_2));
    // register handler for httpServer
    auto httpServer = m_wsService->httpServer();
    if (httpServer)
    {
        httpServer->setHttpReqHandler([this](bcos::boostssl::http::HttpRequest&& _request,
                                          std::function<void(bcos::bytes)> _responseHandler) {
            this->onRPCRequest(std::move(_request), _responseHandler);
        });
    }
    // register the handler for RUN_TASK_METHOD
    m_methodToHandler[RUN_TASK_METHOD] =
        boost::bind(&Rpc::runTask, this, boost::placeholders::_1, boost::placeholders::_2);
    // register the handler for ASYNC_RUN_TASK_METHOD
    m_methodToHandler[ASYNC_RUN_TASK_METHOD] =
        boost::bind(&Rpc::asyncRunTask, this, boost::placeholders::_1, boost::placeholders::_2);
    // register the handler for GET_TASK_STATUS
    m_methodToHandler[GET_TASK_STATUS] =
        boost::bind(&Rpc::getTaskStatus, this, boost::placeholders::_1, boost::placeholders::_2);
    // register ecdh bs mode methods
    m_methodToHandler[ASYNC_RUN_BS_MODE_TASK] = boost::bind(
        &Rpc::asyncRunBsModeTask, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[FETCH_CIPHER] =
        boost::bind(&Rpc::fetchCipher, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[SEND_ECDH_CIPHER] =
        boost::bind(&Rpc::sendEcdhCipher, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[SEND_PARTNER_CIPHER] = boost::bind(
        &Rpc::sendPartnerCipher, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[GET_BS_MODE_TASK_STATUS] = boost::bind(
        &Rpc::getBsModeTaskStatus, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[KILL_BS_MODE_TASK] =
        boost::bind(&Rpc::killBsModeTask, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[UPDATE_BS_MODE_TASK_STATUS] = boost::bind(
        &Rpc::updateBsModeTaskStatus, this, boost::placeholders::_1, boost::placeholders::_2);
    m_methodToHandler[GET_PEERS] =
        boost::bind(&Rpc::getPeers, this, boost::placeholders::_1, boost::placeholders::_2);
    RPC_LOG(INFO) << LOG_DESC("init rpc success") << LOG_KV("selfParty", _selfPartyID);
}

void Rpc::onRPCRequest(
    bcos::boostssl::http::HttpRequest&& _request, std::function<void(bcos::bytes)> _responseHandler)
{
    onRPCRequestImpl(_request.body(), _responseHandler);
}

void Rpc::onRPCRequestImpl(
    std::string_view const& _requestBody, std::function<void(bcos::bytes)> _responseHandler)
{
    auto response = std::make_shared<JsonResponse>();
    try
    {
        auto startT = bcos::utcSteadyTime();
        JsonRequest request(_requestBody);
        if (request.token() != m_token)
        {
            RPC_LOG(INFO) << LOG_DESC("onRPCRequest: token not match")
                          << LOG_KV("interface", request.token()) << LOG_KV("config", m_token);
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)RpcError::NoPermission, "no permission, please check token"));
        }
        response->setJsonRpc(request.jsonRpc());
        response->setId(request.id());
        auto it = m_methodToHandler.find(request.method());
        if (it == m_methodToHandler.end())
        {
            RPC_LOG(DEBUG) << LOG_DESC("onRPCRequest: method not found")
                           << LOG_KV("method", request.method());
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int64_t)RpcError::MethodNotFound, "The method does not exist/is not available."));
        }
        auto const& methodHandler = it->second;
        methodHandler(
            request.params(), [response, _responseHandler, startT, method = request.method()](
                                  Error::Ptr _error, Json::Value&& _result) {
                auto costs = bcos::utcSteadyTime() - startT;
                if (costs >= 300)
                {
                    RPC_LOG(INFO) << LOG_DESC("Slow rpc interface found")
                                  << LOG_KV("method", method) << LOG_KV("costs(ms)", costs);
                }

                if (_error && _error->errorCode() != 0)
                {
                    response->setError(std::move(*_error));
                    response->setResult(std::move(_result));
                }
                else
                {
                    response->setResult(std::move(_result));
                }
                _responseHandler(response->serialize());
            });
    }
    catch (Error const& e)
    {
        RPC_LOG(WARNING) << LOG_DESC("onRPCRequest error") << LOG_KV("request", _requestBody)
                         << LOG_KV("code", e.errorCode()) << LOG_KV("msg", e.errorMessage());
        response->setError(e);
        _responseHandler(response->serialize());
    }
    catch (std::exception const& e)
    {
        RPC_LOG(WARNING) << LOG_DESC("onRPCRequest error") << LOG_KV("request", _requestBody)
                         << LOG_KV("msg", boost::diagnostic_information(e));
        response->mutableError()->setErrorCode(PPCRetCode::EXCEPTION);
        response->mutableError()->setErrorMessage(boost::diagnostic_information(e));
        _responseHandler(response->serialize());
    }
}

void Rpc::onWsRpcRequest(std::shared_ptr<bcos::boostssl::MessageFace> _msg,
    std::shared_ptr<bcos::boostssl::ws::WsSession> _session)
{
    auto buffer = _msg->payload();
    auto request = std::string_view((const char*)buffer->data(), buffer->size());
    onRPCRequestImpl(request, [m_buffer = std::move(buffer), _msg, _session](bcos::bytes resp) {
        if (_session && _session->isConnected())
        {
            auto buffer = std::make_shared<bcos::bytes>(std::move(resp));
            _msg->setPayload(buffer);
            _session->asyncSendMessage(_msg);
            return;
        }
        RPC_LOG(WARNING)
            << LOG_DESC("onWsRpcRequest: unable to send response for session has been inactive")
            << LOG_KV("req", std::string_view((const char*)m_buffer->data(), m_buffer->size()))
            << LOG_KV("resp", std::string_view((const char*)resp.data(), resp.size()))
            << LOG_KV("seq", _msg->seq())
            << LOG_KV("endpoint", _session ? _session->endPoint() : std::string(""));
    });
}

void Rpc::runTask(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_rpcStorage)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::StorageNotSet, "storage for rpc not set"));
    }
    auto task = m_taskFactory->createTask(_req);
    auto taskHandler = getTaskHandler(task->type(), task->algorithm());
    // not find the handler
    if (!taskHandler)
    {
        std::stringstream oss;
        oss << "The specified task algorithm not been implemented or disabled, type: "
            << (ppc::protocol::TaskType)(task->type()) << "(" << std::to_string(task->type())
            << ")";
        oss << ", algorithm: " << (ppc::protocol::TaskAlgorithmType)(task->algorithm()) << "("
            << std::to_string(task->algorithm()) << ")";
        auto errorMsg = oss.str();
        RPC_LOG(WARNING) << errorMsg;
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)RpcError::NotImplemented, errorMsg));
    }
    RPC_LOG(DEBUG) << LOG_DESC("runTask") << printTaskInfo(task);
    auto startT = utcSteadyTime();

    // insert task
    auto error = m_rpcStorage->insertTask(task);
    if (error && error->errorCode())
    {
        auto result = std::make_shared<ppc::protocol::TaskResult>(task->id());
        result->setStatus(toString(TaskStatus::RUNNING));
        _respFunc(result->error(), result->serializeToJson());
        return;
    }

    taskHandler(task, [&, task, _respFunc, startT](ppc::protocol::TaskResult::Ptr&& _result) {
        auto cost = bcos::utcSteadyTime() - startT;
        _result->setTimeCost(cost);
        RPC_LOG(DEBUG) << LOG_DESC("runTask finish") << LOG_KV("timecost", cost)
                       << printTaskInfo(task);

        // update status
        auto error = m_rpcStorage->updateTaskStatus(_result);
        if (error && error->errorCode())
        {
            auto result = std::make_shared<ppc::protocol::TaskResult>(task->id());
            result->setError(std::move(error));
            result->setStatus(toString(TaskStatus::FAILED));
            _respFunc(result->error(), result->serializeToJson());
            return;
        }
        _respFunc(_result->error(), _result->serializeToJson());
    });
}

void Rpc::asyncRunTask(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_rpcStorage)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::StorageNotSet, "storage for rpc not set"));
    }
    auto task = m_taskFactory->createTask(_req);
    auto taskHandler = getTaskHandler(task->type(), task->algorithm());
    // not find the handler
    if (!taskHandler)
    {
        std::stringstream oss;
        oss << "The specified task algorithm not been implemented or disabled, type: "
            << (ppc::protocol::TaskType)(task->type()) << "(" << std::to_string(task->type())
            << ")";
        oss << ", algorithm: " << (ppc::protocol::TaskAlgorithmType)(task->algorithm()) << "("
            << std::to_string(task->algorithm()) << ")";
        auto errorMsg = oss.str();
        RPC_LOG(WARNING) << errorMsg;
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)RpcError::NotImplemented, errorMsg));
    }
    RPC_LOG(DEBUG) << LOG_DESC("asyncRunTask") << printTaskInfo(task);

    // insert task
    auto error = m_rpcStorage->insertTask(task);
    if (error && error->errorCode())
    {
        auto result = std::make_shared<ppc::protocol::TaskResult>(task->id());
        result->setStatus(toString(TaskStatus::RUNNING));
        _respFunc(result->error(), result->serializeToJson());
        return;
    }

    // return response immediately
    auto result = std::make_shared<ppc::protocol::TaskResult>(task->id());
    result->setStatus(toString(TaskStatus::RUNNING));
    _respFunc(result->error(), result->serializeToJson());

    auto startT = utcSteadyTime();
    taskHandler(task, [&, task, startT](ppc::protocol::TaskResult::Ptr&& _result) {
        auto cost = utcSteadyTime() - startT;
        _result->setTimeCost(cost);
        RPC_LOG(DEBUG) << LOG_DESC("runTask finish") << LOG_KV("timecost", cost)
                       << printTaskInfo(task);
        // update status
        auto error = m_rpcStorage->updateTaskStatus(_result);
        if (error && error->errorCode())
        {
            RPC_LOG(WARNING) << LOG_DESC("failed to updateTaskStatus") << printTaskInfo(task);
        }
    });
}

void Rpc::getTaskStatus(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_rpcStorage)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::StorageNotSet, "storage for rpc not set"));
    }

    if (!_req.isMember("taskID"))
    {
        BOOST_THROW_EXCEPTION(InvalidParameter() << errinfo_comment("Must specify the taskID"));
    }
    auto taskID = _req["taskID"].asString();

    auto result = m_rpcStorage->getTaskStatus(taskID);
    _respFunc(result->error(), result->serializeToJson());
}


void Rpc::asyncRunBsModeTask(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }
    auto request = std::make_shared<psi::RunTaskRequest>(_req, m_prePath);
    m_bsEcdhPSI->asyncRunTask(request, [&](psi::BsEcdhResult::Ptr&& _result) {
        _respFunc(_result->error(), _result->serializeToJson());
    });
}

void Rpc::fetchCipher(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }

    auto request = std::make_shared<psi::FetchCipherRequest>(_req);
    auto result = m_bsEcdhPSI->fetchCipher(request);
    _respFunc(result->error(), result->serializeToJson());
}

void Rpc::sendEcdhCipher(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }

    auto request = std::make_shared<psi::SendEcdhCipherRequest>(_req);
    auto result = m_bsEcdhPSI->sendEcdhCipher(request);
    _respFunc(result->error(), result->serializeToJson());
}

void Rpc::getPeers(Json::Value const& _req, RespFunc _respFunc)
{
    if (m_gateway == nullptr)
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR(-1, "the gateway not initialized!"));
    }
    m_gateway->asyncGetPeers([_respFunc](bcos::Error::Ptr error, std::string peersInfo) {
        try
        {
            Json::Value root;
            Json::Reader jsonReader;

            if (!jsonReader.parse(peersInfo, root))
            {
                BOOST_THROW_EXCEPTION(BCOS_ERROR(-1, "Invalid json string: " + peersInfo));
            }
            _respFunc(error, std::move(root));
        }
        catch (std::exception const& e)
        {
            RPC_LOG(WARNING) << LOG_DESC("getPeers exception")
                             << LOG_KV("error", boost::diagnostic_information(e));
        }
    });
}

void Rpc::sendPartnerCipher(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }

    auto request = std::make_shared<psi::SendPartnerCipherRequest>(_req);
    auto result = m_bsEcdhPSI->sendPartnerCipher(request);
    _respFunc(result->error(), result->serializeToJson());
}

void Rpc::getBsModeTaskStatus(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }

    auto request = std::make_shared<psi::GetTaskStatusRequest>(_req);
    auto result = m_bsEcdhPSI->getTaskStatus(request);
    _respFunc(result->error(), result->serializeToJson());
}

void Rpc::killBsModeTask(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }

    auto request = std::make_shared<psi::KillTaskRequest>(_req);
    auto result = m_bsEcdhPSI->killTask(request);
    _respFunc(result->error(), result->serializeToJson());
}

void Rpc::updateBsModeTaskStatus(Json::Value const& _req, RespFunc _respFunc)
{
    if (!m_bsEcdhPSI)
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int64_t)RpcError::BsModePsiNotSet, "bs psi for rpc not set"));
    }

    auto request = std::make_shared<psi::UpdateTaskStatusRequest>(_req);
    auto result = m_bsEcdhPSI->updateTaskStatus(request);
    _respFunc(result->error(), result->serializeToJson());
}
