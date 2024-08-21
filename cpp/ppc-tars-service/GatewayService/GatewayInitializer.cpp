/**
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
 * @file GatewayInitializer.cpp
 * @author: shawnhe
 * @date 2022-10-21
 */

#include "GatewayInitializer.h"

using namespace ppctars;
using namespace ppc;
using namespace ppc::front;
using namespace ppc::gateway;
using namespace ppc::storage;
using namespace ppc::protocol;

using namespace bcos;
using namespace bcos::boostssl;
using namespace bcos::boostssl::ws;

void GatewayInitializer::start()
{
    if (m_gateway)
    {
        m_gateway->start();
    }
}

void GatewayInitializer::stop()
{
    if (m_gateway)
    {
        m_gateway->stop();
    }
}

void GatewayInitializer::init(ppc::protocol::NodeArch _arch)
{
    GATEWAY_LOG(INFO) << LOG_BADGE("init gateway") << LOG_KV("arch", _arch);
    m_gateway = m_gatewayFactory->buildGateway(
        _arch, m_gatewayConfig, m_cache, m_messageFactory, m_threadPool);
    GATEWAY_LOG(INFO) << LOG_BADGE("init gateway success");
}