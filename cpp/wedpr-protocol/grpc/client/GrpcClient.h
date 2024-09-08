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
 * @file GrpcClient.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "Service.grpc.pb.h"
#include "ServiceHealth.grpc.pb.h"
#include "ppc-framework/protocol/GrpcConfig.h"
#include "wedpr-protocol/grpc/Common.h"
#include <grpcpp/grpcpp.h>

namespace ppc::protocol
{
// refer to: https://grpc.io/docs/languages/cpp/callback/
class GrpcClient
{
public:
    using Ptr = std::shared_ptr<GrpcClient>;
    GrpcClient(ppc::protocol::GrpcConfig::Ptr const& grpcConfig, std::string const& endPoints)
      : m_channel(grpc::CreateCustomChannel(
            endPoints, grpc::InsecureChannelCredentials(), toChannelConfig(grpcConfig))),
        m_healthCheckStub(grpc::health::v1::Health::NewStub(m_channel))
    {}

    virtual ~GrpcClient() = default;

    std::shared_ptr<grpc::Channel> const& channel() { return m_channel; }

    bool checkHealth();

protected:
    std::shared_ptr<grpc::Channel> m_channel;
    // the healthcheck stub
    std::unique_ptr<grpc::health::v1::Health::Stub> m_healthCheckStub;
};
}  // namespace ppc::protocol