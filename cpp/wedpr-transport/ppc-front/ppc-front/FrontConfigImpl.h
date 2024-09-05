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
 * @file FrontConfigImpl.h
 * @author: yujiechen
 * @date 2024-08-22
 */

#pragma once
#include "ppc-framework/front/FrontConfig.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include <bcos-utilities/Common.h>

namespace ppc::front
{
class FrontConfigImpl : public FrontConfig
{
public:
    using Ptr = std::shared_ptr<FrontConfigImpl>;
    FrontConfigImpl(ppc::protocol::INodeInfoFactory::Ptr nodeInfoFactory)
      : FrontConfig(), m_nodeInfoFactory(std::move(nodeInfoFactory))
    {}
    FrontConfigImpl(ppc::protocol::INodeInfoFactory::Ptr nodeInfoFactory, int threadPoolSize,
        std::string nodeID)
      : FrontConfig(threadPoolSize, nodeID), m_nodeInfoFactory(std::move(nodeInfoFactory))
    {}

    ~FrontConfigImpl() override = default;

    ppc::protocol::INodeInfo::Ptr generateNodeInfo() const override
    {
        auto nodeInfo = m_nodeInfoFactory->build(
            bcos::bytesConstRef((bcos::byte*)m_nodeID.data(), m_nodeID.size()),
            m_selfEndPoint.entryPoint());
        nodeInfo->setComponents(std::set<std::string>(m_components.begin(), m_components.end()));
        return nodeInfo;
    }

private:
    ppc::protocol::INodeInfoFactory::Ptr m_nodeInfoFactory;
};

class FrontConfigBuilderImpl : public FrontConfigBuilder
{
public:
    using Ptr = std::shared_ptr<FrontConfigBuilderImpl>;
    FrontConfigBuilderImpl(ppc::protocol::INodeInfoFactory::Ptr nodeInfoFactory)
      : m_nodeInfoFactory(nodeInfoFactory)
    {}
    ~FrontConfigBuilderImpl() override = default;

    FrontConfig::Ptr build() const override
    {
        return std::make_shared<FrontConfigImpl>(m_nodeInfoFactory);
    }
    FrontConfig::Ptr build(int threadPoolSize, std::string nodeID) const override
    {
        return std::make_shared<FrontConfigImpl>(
            m_nodeInfoFactory, threadPoolSize, std::move(nodeID));
    }

private:
    ppc::protocol::INodeInfoFactory::Ptr m_nodeInfoFactory;
};
}  // namespace ppc::front
