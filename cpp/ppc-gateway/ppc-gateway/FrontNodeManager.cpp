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
 * @file FrontNodeManager.cpp
 * @author: shawnhe
 * @date 2022-10-23
 */

#include "FrontNodeManager.h"

using namespace bcos;
using namespace ppc::gateway;
using namespace ppc::front;

FrontInterface::Ptr FrontNodeManager::getFront(const std::string& _serviceEndpoint)
{
    bcos::ReadGuard lock(x_frontNodes);
    auto it = m_frontNodes.find(_serviceEndpoint);
    if (it != m_frontNodes.end())
    {
        return it->second;
    }
    return nullptr;
}

void FrontNodeManager::registerFront(
    std::string const& _endPoint, front::FrontInterface::Ptr _front)
{
    bcos::UpgradableGuard l;
    if (m_frontNodes.count(_endPoint))
    {
        return;
    }
    bcos::UpgradeGuard ul(l);
    m_frontNodes[_endPoint] = _front;
    GATEWAY_LOG(INFO) << LOG_DESC("registerFront success") << LOG_KV("endPoint", _endPoint);
}

void FrontNodeManager::unregisterFront(std::string const& _endPoint)
{
    bcos::UpgradableGuard l;
    auto it = m_frontNodes.find(_endPoint);
    if (it == m_frontNodes.end())
    {
        return;
    }
    bcos::UpgradeGuard ul(l);
    m_frontNodes.erase(it);
    GATEWAY_LOG(INFO) << LOG_DESC("unregisterFront success") << LOG_KV("endPoint", _endPoint);
}
