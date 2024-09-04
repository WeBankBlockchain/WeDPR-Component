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
 * @file TransportImpl.h
 * @author: yujiechen
 * @date 2024-09-04
 */
#pragma once
#include "Transport.h"
#include "ppc-framework/gateway/IGateway.h"
#include "ppc-front/FrontFactory.h"
#include "protobuf/NodeInfoImpl.h"
#include "protocol/src/v1/MessageHeaderImpl.h"
#include "protocol/src/v1/MessagePayloadImpl.h"

namespace ppc::sdk
{
class TransportImpl : public Transport
{
public:
    TransportImpl(ppc::Front::FrontConfig::Ptr config, ppc::gateway::IGateway::Ptr const& gateway)
      : m_config(std::move(config))
    {
        FrontFactory frontFactory;
        m_front = frontFactory.build(std::make_shared<NodeInfoFactory>(),
            std::make_shared<MessagePayloadBuilderImpl>(),
            std::make_shared<MessageOptionalHeaderBuilderImpl>(), gateway, m_config);
    }
    ~TransportImpl() override = default;

protected:
    ppc::Front::FrontConfig::Ptr m_config;
};
}  // namespace ppc::sdk