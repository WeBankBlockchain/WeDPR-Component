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
 * @file INodeDiscovery.h
 * @author: yujiechen
 * @date 2024-11-06
 */
#pragma once
#include "ppc-framework/protocol/INodeInfo.h"
#include <memory>

namespace ppc::front
{
class INodeDiscovery
{
public:
    using Ptr = std::shared_ptr<INodeDiscovery>;
    INodeDiscovery() = default;
    virtual ~INodeDiscovery() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    // Note: use std::shared_ptr here for swig wrapper
    virtual std::vector<std::shared_ptr<ppc::protocol::INodeInfo>> getAliveNodeList() const = 0;
};
}  // namespace ppc::front