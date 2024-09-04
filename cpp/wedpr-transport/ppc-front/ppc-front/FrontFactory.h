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
 * @file FrontFactory.h
 * @author: yujiechen
 * @date 2024-9-04
 */
#pragma once
#include "ppc-framework/front/FrontConfig.h"
#include "ppc-framework/front/IFront.h"
#include "ppc-framework/gateway/IGateway.h"
#include "ppc-framework/protocol/INodeInfo.h"

namespace ppc::front
{
class FrontFactory
{
public:
    using Ptr = std::shared_ptr<FrontFactory>;
    FrontFactory() = default;
    virtual ~FrontFactory() = default;

    IFront::Ptr build(ppc::protocol::INodeInfoFactory::Ptr nodeInfoFactory,
        ppc::protocol::MessagePayloadBuilder::Ptr messageFactory,
        ppc::protocol::MessageOptionalHeaderBuilder::Ptr routerInfoBuilder,
        ppc::gateway::IGateway::Ptr const& gateway, FrontConfig::Ptr config);
};
}  // namespace ppc::front