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
 * @file FrontNodeManager.h
 * @author: shawnhe
 * @date 2022-10-23
 */

#pragma once
#include "Common.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-front/ppc-front/Front.h"
#include <memory>
#include <utility>

namespace ppc::gateway
{
class FrontNodeManager : public std::enable_shared_from_this<FrontNodeManager>
{
public:
    using Ptr = std::shared_ptr<FrontNodeManager>;
    FrontNodeManager() = default;
    virtual ~FrontNodeManager() = default;

    front::FrontInterface::Ptr getFront(const std::string& _serviceEndpoint);
    virtual void registerFront(std::string const& _endPoint, front::FrontInterface::Ptr _front);

    virtual void unregisterFront(std::string const& _endPoint);

    virtual std::unordered_map<std::string, front::FrontInterface::Ptr> getAllFront() const
    {
        bcos::ReadGuard l(x_frontNodes);
        return m_frontNodes;
    }

private:
    // key: serviceEndpoint, value: FrontInterface
    std::unordered_map<std::string, front::FrontInterface::Ptr> m_frontNodes;
    mutable bcos::SharedMutex x_frontNodes;
};

}  // namespace ppc::gateway
