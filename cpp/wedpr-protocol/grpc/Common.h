/**
 *  Copyright (C) 2021 FISCO BCOS.
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
 * @file Common.h
 * @author: yujiechen
 * @date 2021-04-12
 */
#pragma once
#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/GrpcConfig.h"
#include <grpc/compression.h>
#include <grpcpp/grpcpp.h>

namespace ppc::protocol
{
#define GRPC_LOG(LEVEL) BCOS_LOG(LEVEL) << "[GRPC]"

inline grpc::ChannelArguments toChannelConfig(ppc::protocol::GrpcConfig::Ptr const& grpcConfig)
{
    grpc::ChannelArguments args;
    if (grpcConfig == nullptr)
    {
        return args;
    }
    args.SetLoadBalancingPolicyName(grpcConfig->loadBalancePolicy());
    if (grpcConfig->enableHealthCheck())
    {
        args.SetServiceConfigJSON(
            "{\"healthCheckConfig\": "
            "{\"serviceName\": \"\"}}");
    }
    // disable dns lookup
    if (!grpcConfig->enableDnslookup())
    {
        args.SetInt("grpc.enable_dns_srv_lookup", 0);
    }
    else
    {
        args.SetInt("grpc.enable_dns_srv_lookup", 1);
    }
    args.SetMaxReceiveMessageSize(grpcConfig->maxReceivedMessageSize());
    args.SetMaxSendMessageSize(grpcConfig->maxSendMessageSize());
    // the compress algorithm
    args.SetCompressionAlgorithm((grpc_compression_algorithm)(grpcConfig->compressAlgorithm()));
    GRPC_LOG(INFO) << LOG_DESC("toChannelConfig") << printGrpcConfig(grpcConfig);
    return args;
}
}  // namespace ppc::protocol