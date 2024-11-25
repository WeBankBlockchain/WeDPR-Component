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
 * @file Rpc.h
 * @author: yujiechen
 * @date 2022-11-3
 */
#pragma once
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/gateway/IGateway.h"
#include "ppc-framework/rpc/RpcInterface.h"
#include "ppc-framework/rpc/RpcStatusInterface.h"
#include "protocol/src/JsonTaskImpl.h"
#include <bcos-boostssl/httpserver/Common.h>
#include <bcos-boostssl/interfaces/MessageFace.h>
#include <bcos-boostssl/websocket/WsService.h>
#include <bcos-utilities/Common.h>
#include <ppc-psi/src/bs-ecdh-psi/BsEcdhPSIInterface.h>
#include <memory>

namespace ppc::rpc
{
class Rpc : public RpcInterface
{
public:
    using Ptr = std::shared_ptr<Rpc>;
    Rpc(std::shared_ptr<bcos::boostssl::ws::WsService> _wsService,
        ppc::gateway::IGateway::Ptr gateway, std::string const& _selfPartyID,
        std::string const& _token, std::string const& _prePath = "data");
    ~Rpc() override { stop(); }
    void start() override
    {
        if (m_wsService)
        {
            m_wsService->start();
        }
        if (m_rpcStorage)
        {
            m_rpcStorage->start();
        }
        RPC_LOG(INFO) << LOG_DESC("start RPC");
    }
    void stop() override
    {
        if (m_wsService)
        {
            m_wsService->stop();
        }
        if (m_rpcStorage)
        {
            m_rpcStorage->stop();
        }
        RPC_LOG(INFO) << LOG_DESC("stop RPC");
    }

    void registerHandler(std::string const& _methodName,
        std::function<void(Json::Value const&, RespFunc)> _handler) override
    {
        bcos::UpgradableGuard l(x_methodToHandler);
        if (m_methodToHandler.count(_methodName))
        {
            RPC_LOG(INFO) << LOG_DESC("registerHandler return for method handler already exists")
                          << LOG_KV("method", _methodName);
            return;
        }
        bcos::UpgradeGuard ul(l);
        m_methodToHandler[_methodName] = _handler;
        RPC_LOG(INFO) << LOG_DESC("registerHandler success") << LOG_KV("method", _methodName);
    }

    void registerTaskHandler(
        ppc::protocol::TaskType _type, uint8_t _algorithm, TaskHandler _handler) override
    {
        bcos::UpgradableGuard l(x_taskHandlers);
        if (m_taskHandlers.count((uint8_t)_type) &&
            m_taskHandlers.at((uint8_t)_type).count(_algorithm))
        {
            RPC_LOG(INFO) << LOG_DESC("registerTaskHandler return for handler already exists")
                          << LOG_KV("type", _type) << LOG_KV("algorithm", _algorithm);
            return;
        }
        m_taskHandlers[(uint8_t)_type][_algorithm] = _handler;
        RPC_LOG(INFO) << LOG_DESC("registerTaskHandler success") << LOG_KV("type", _type)
                      << LOG_KV("algorithm", (int)_algorithm);
    }

    void setRpcStorage(RpcStatusInterface::Ptr _storage) { m_rpcStorage = std::move(_storage); }

    void setBsEcdhPSI(psi::BsEcdhPSIInterface::Ptr _psi) { m_bsEcdhPSI = std::move(_psi); }

protected:
    virtual void onRPCRequest(bcos::boostssl::http::HttpRequest&& _request,
        std::function<void(bcos::bytes)> _responseHandler);

    virtual void onRPCRequestImpl(
        std::string_view const& _requestBody, std::function<void(bcos::bytes)> _responseHandler);
    virtual void onWsRpcRequest(std::shared_ptr<bcos::boostssl::MessageFace> _msg,
        std::shared_ptr<bcos::boostssl::ws::WsSession> _session);

    virtual TaskHandler getTaskHandler(uint8_t _type, uint8_t _algorithm) const
    {
        bcos::ReadGuard l(x_taskHandlers);
        if (!m_taskHandlers.count(_type) || !m_taskHandlers.at(_type).count(_algorithm))
        {
            return nullptr;
        }
        return m_taskHandlers.at(_type).at(_algorithm);
    }

    virtual void runTask(Json::Value const& _req, RespFunc _respFunc);
    virtual void asyncRunTask(Json::Value const& _req, RespFunc _respFunc);
    virtual void getTaskStatus(Json::Value const& _req, RespFunc _respFunc);

    virtual void asyncRunBsModeTask(Json::Value const& _req, RespFunc _respFunc);
    virtual void fetchCipher(Json::Value const& _req, RespFunc _respFunc);
    virtual void sendEcdhCipher(Json::Value const& _req, RespFunc _respFunc);
    virtual void sendPartnerCipher(Json::Value const& _req, RespFunc _respFunc);
    virtual void getBsModeTaskStatus(Json::Value const& _req, RespFunc _respFunc);
    virtual void killBsModeTask(Json::Value const& _req, RespFunc _respFunc);
    virtual void updateBsModeTaskStatus(Json::Value const& _req, RespFunc _respFunc);

    virtual void getPeers(Json::Value const& _req, RespFunc _respFunc);

private:
    std::string m_prePath;
    std::shared_ptr<bcos::boostssl::ws::WsService> m_wsService;
    ppc::gateway::IGateway::Ptr m_gateway;
    RpcStatusInterface::Ptr m_rpcStorage;

    // Note: here use jsonTaskFactory to decrease the overhead to convert json::value to string when
    // deserialze the task
    ppc::protocol::JsonTaskFactory::Ptr m_taskFactory;
    std::string m_token;

    // the method_name to function
    std::map<std::string, std::function<void(Json::Value const&, RespFunc)>> m_methodToHandler;
    bcos::SharedMutex x_methodToHandler;

    // the taskHandlers
    std::map<uint8_t, std::map<uint8_t, TaskHandler>> m_taskHandlers;
    mutable bcos::SharedMutex x_taskHandlers;

    psi::BsEcdhPSIInterface::Ptr m_bsEcdhPSI;
};
}  // namespace ppc::rpc
