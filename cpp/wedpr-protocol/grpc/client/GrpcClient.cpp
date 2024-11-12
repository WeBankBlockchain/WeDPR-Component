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
 * @file GrpcClient.cpp
 * @author: yujiechen
 * @date 2024-09-02
 */
#include "GrpcClient.h"
#include "Common.h"
#include <memory>

using namespace ppc::protocol;
using namespace ppc::proto;
using namespace grpc;
using namespace grpc::health::v1;

GrpcClient::GrpcClient(
    ppc::protocol::GrpcConfig::Ptr const& grpcConfig, std::string const& endPoints)
  : m_grpcConfig(grpcConfig),
    m_channel(grpc::CreateCustomChannel(
        endPoints, grpc::InsecureChannelCredentials(), toChannelConfig(grpcConfig))),
    m_healthCheckStub(grpc::health::v1::Health::NewStub(m_channel))
{
    std::vector<std::string> endPointList;
    boost::split(endPointList, endPoints, boost::is_any_of(","));
    // create the broadcast channels
    for (auto const& endPoint : endPointList)
    {
        GRPC_CLIENT_LOG(INFO) << LOG_DESC("create broadcast-channel, endpoint: ") << endPoint;
        m_broadcastChannels.push_back(
            {endPoint, grpc::CreateCustomChannel(endPoint, grpc::InsecureChannelCredentials(),
                           toChannelConfig(grpcConfig))});
    }
}

bool GrpcClient::checkHealth()
{
    try
    {
        HEALTH_LOG(TRACE) << LOG_DESC("checkHealth");
        ClientContext context;
        HealthCheckResponse response;
        auto status =
            m_healthCheckStub->Check(&context, HealthCheckRequest::default_instance(), &response);
        if (!status.ok())
        {
            GRPC_CLIENT_LOG(WARNING)
                << LOG_DESC("GrpcClient check health failed") << LOG_KV("code", status.error_code())
                << LOG_KV("msg", status.error_message());
            return false;
        }
        return true;
    }
    catch (std::exception const& e)
    {
        GRPC_CLIENT_LOG(WARNING) << LOG_DESC("GrpcClient check health exception")
                                 << LOG_KV("error", boost::diagnostic_information(e));
        return false;
    }
}

bcos::Error::Ptr GrpcClient::broadCast(
    std::function<bcos::Error::Ptr(ChannelInfo const& channel)> callback)
{
    auto result = std::make_shared<bcos::Error>(0, "");
    for (auto const& channel : m_broadcastChannels)
    {
        try
        {
            if (channel.channel->GetState(false) == GRPC_CHANNEL_SHUTDOWN)
            {
                GRPC_CLIENT_LOG(INFO) << LOG_DESC("Ignore unconnected channel")
                                      << LOG_KV("endpoint", channel.endPoint);
                continue;
            }
            auto error = callback(channel);
            if (error && error->errorCode() != 0)
            {
                result->setErrorCode(error->errorCode());
                result->setErrorMessage(result->errorMessage() + error->errorMessage() + "; ");
            }
        }
        catch (std::exception const& e)
        {
            GRPC_CLIENT_LOG(WARNING)
                << LOG_DESC("GrpcClient broadCast exception") << LOG_KV("remote", channel.endPoint)
                << LOG_KV("error", boost::diagnostic_information(e));
        }
    }
    return result;
}