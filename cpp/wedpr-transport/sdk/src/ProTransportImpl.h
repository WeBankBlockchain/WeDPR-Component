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
#include "bcos-utilities/Timer.h"
#include "protocol/src/v1/MessageImpl.h"

namespace ppc::protocol
{
class GrpcServer;
class FrontServer;
}  // namespace ppc::protocol


namespace ppc::sdk
{
class ProTransportImpl : public Transport, public std::enable_shared_from_this<ProTransportImpl>
{
public:
    using Ptr = std::shared_ptr<ProTransportImpl>;
    ProTransportImpl(ppc::front::FrontConfig::Ptr config, int keepAlivePeriodMs = 3000);
    ~ProTransportImpl();
    void start() override;
    void stop() override;

protected:
    void keepAlive();

protected:
    std::shared_ptr<ppc::protocol::GrpcServer> m_server;
    std::shared_ptr<ppc::protocol::FrontServer> m_frontService;
    int m_keepAlivePeriodMs;
    std::shared_ptr<bcos::Timer> m_timer;
};
}  // namespace ppc::sdk