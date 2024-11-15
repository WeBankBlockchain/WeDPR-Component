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
 * @file GrpcServer.cpp
 * @author: yujiechen
 * @date 2024-09-03
 */
#include "GrpcServer.h"
#include "Common.h"
#include "grpcpp/ext/proto_server_reflection_plugin.h"

using namespace ppc;
using namespace ppc::protocol;
using namespace grpc;

void GrpcServer::start()
{
    if (m_running)
    {
        GRPC_SERVER_LOG(INFO) << LOG_DESC("GrpcServer has already been started!")
                              << LOG_KV("listenEndPoint", m_config->listenEndPoint());
        return;
    }
    m_running = true;
    if (m_config->enableHealthCheck())
    {
        grpc::EnableDefaultHealthCheckService(true);
    }
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    grpc::ServerBuilder builder;
    // disable port reuse
    builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
    // without authentication
    builder.AddListeningPort(m_config->listenEndPoint(), grpc::InsecureServerCredentials());
    builder.SetMaxMessageSize(m_config->grpcConfig()->maxMsgSize());
    builder.SetMaxSendMessageSize(m_config->grpcConfig()->maxSendMessageSize());
    builder.SetMaxReceiveMessageSize(m_config->grpcConfig()->maxReceivedMessageSize());
    // register the service
    for (auto const& service : m_bindingServices)
    {
        builder.RegisterService(service.get());
    }
    m_server = std::unique_ptr<Server>(builder.BuildAndStart());
    if (!m_server)
    {
        GRPC_SERVER_LOG(INFO) << LOG_DESC(
                                     "GrpcServer BuildAndStart failed, please check the port has "
                                     "been occupied or not")
                              << LOG_KV("listenEndPoint", m_config->listenEndPoint());
        BOOST_THROW_EXCEPTION(
            WeDPRException() << bcos::errinfo_comment("BuildAndStart grpcServer failed for bind "
                                                      "error, please check the listenPort, current "
                                                      "listenEndPoint: " +
                                                      m_config->listenEndPoint()));
    }
    GRPC_SERVER_LOG(INFO) << LOG_DESC("GrpcServer start success!")
                          << LOG_KV("listenEndPoint", m_config->listenEndPoint())
                          << LOG_KV("maxMsgSize", m_config->grpcConfig()->maxMsgSize())
                          << LOG_KV(
                                 "maxSendMessageSize", m_config->grpcConfig()->maxSendMessageSize())
                          << LOG_KV("maxReceivedMessageSize",
                                 m_config->grpcConfig()->maxReceivedMessageSize());
}

void GrpcServer::stop()
{
    if (!m_running)
    {
        GRPC_SERVER_LOG(INFO) << LOG_DESC("GrpcServer has already been stopped!");
        return;
    }
    m_running = false;
    if (m_server)
    {
        m_server->Shutdown();
    }
    GRPC_SERVER_LOG(INFO) << LOG_DESC("GrpcServer stop success!");
}