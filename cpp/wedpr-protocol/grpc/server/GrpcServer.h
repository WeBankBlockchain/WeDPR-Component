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
 * @file GrpcServer.h
 * @author: yujiechen
 * @date 2024-09-03
 */
#pragma once
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

namespace ppc::protocol
{
// refer to: https://grpc.io/docs/languages/cpp/callback/
struct GrpcServerConfig
{
    std::string listenIp;
    int listenPort;

    std::string endPoint() const { return listenIp + ":" + std::to_string(listenPort); }
};
class GrpcServer
{
public:
    using Ptr = std::shared_ptr<GrpcServer>;
    GrpcServer(GrpcServerConfig const& config) : m_config(config) {}
    virtual ~GrpcServer() = default;

    virtual void start();
    virtual void stop();

    virtual void registerService(std::shared_ptr<grpc::Service> service)
    {
        m_bindingServices.emplace_back(std::move(service));
    }

private:
    bool m_running = false;
    GrpcServerConfig m_config;

    std::unique_ptr<grpc::Server> m_server;
    std::vector<std::shared_ptr<grpc::Service>> m_bindingServices;
};
}  // namespace ppc::protocol