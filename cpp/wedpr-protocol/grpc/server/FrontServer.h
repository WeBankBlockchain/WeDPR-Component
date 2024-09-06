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
 * @file FrontServer.h
 * @author: yujiechen
 * @date 2024-09-03
 */
#pragma once
#include "Service.grpc.pb.h"
#include <grpcpp/health_check_service_interface.h>
#include <ppc-framework/front/IFront.h>
#include <ppc-framework/protocol/Message.h>
#include <memory>

namespace ppc::protocol
{
class FrontServer : public ppc::proto::Front::CallbackService
{
public:
    using Ptr = std::shared_ptr<FrontServer>;
    FrontServer(ppc::protocol::MessageBuilder::Ptr msgBuilder, ppc::front::IFront::Ptr front)
      : m_msgBuilder(std::move(msgBuilder)), m_front(std::move(front))
    {}
    ~FrontServer() override = default;

    grpc::ServerUnaryReactor* onReceiveMessage(grpc::CallbackServerContext* context,
        const ppc::proto::ReceivedMessage* receivedMsg, ppc::proto::Error* reply) override;

    void setHealthCheckService(grpc::HealthCheckServiceInterface* healthCheckService)
    {
        m_healthCheckService = healthCheckService;
        m_healthCheckService->SetServingStatus(true);
    }

private:
    grpc::HealthCheckServiceInterface* m_healthCheckService = nullptr;
    ppc::front::IFront::Ptr m_front;
    ppc::protocol::MessageBuilder::Ptr m_msgBuilder;
};
}  // namespace ppc::protocol