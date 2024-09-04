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
 * @file ProTransportImpl.h
 * @author: yujiechen
 * @date 2024-09-04
 */
#pragma once

#include "TransportImpl.h"
#include "wedpr-protocol/grpc/client/GatewayClient.h"
#include "wedpr-protocol/grpc/server/GrpcServer.h"

namespace ppc::sdk
{
class ProTransportImpl : public Transport
{
public:
    using Ptr = std::shared_ptr<ProTransportImpl>;
    ProTransportImpl(ppc::Front::FrontConfig::Ptr config);

    void start() override
    {
        m_server->start();
        m_front->start();
    }
    void stop() override
    {
        m_server->stop();
        m_front->stop();
    }

protected:
    ppc::protocol::GrpcServer::Ptr m_server;
};
}  // namespace ppc::sdk