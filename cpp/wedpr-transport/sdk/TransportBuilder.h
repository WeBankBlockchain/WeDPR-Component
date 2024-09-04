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
 * @file TransportBuilder.h
 * @author: yujiechen
 * @date 2024-09-04
 */
#pragma once
#include "ProTransportImpl.h"
#include "Transport.h"
#include "TransportImpl.h"
#include <memory>
namespace ppc::sdk
{
enum class SDKMode : uint8_t
{
    AIR = 0x00,
    PRO = 0x01,
};
class TransportBuilder
{
public:
    using Ptr = std::shared_ptr<TransportBuilder>;
    TransportBuilder() = default;
    virtual ~TransportBuilder() = default;

    Transport::Ptr build(SDKMode mode, ppc::Front::FrontConfig::Ptr config,
        ppc::gateway::IGateway::Ptr const& gateway)
    {
        switch (mode)
        {
        case SDKMode::AIR:
        {
            return std::make_shared<TransportImpl>(config, gateway);
        }
        case SDKMode::PRO:
        {
            return std::make_shared<ProTransportImpl>(config);
        }
        default:
            throw std::exception("Unsupported sdk mode, only support AIR/PRO mode!");
        }
    }
};
}  // namespace ppc::sdk