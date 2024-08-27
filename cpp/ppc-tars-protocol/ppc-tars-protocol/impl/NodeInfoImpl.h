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
 * @file NodeInfoImpl.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-framework/protocol/INodeInfo.h"
#include "ppc-tars-protocol/tars/NodeInfo.h"
#include <memory>
namespace ppc::protocol
{
// the node information
class NodeInfoImpl : public INodeInfo
{
public:
    using Ptr = std::shared_ptr<NodeInfoImpl>;
    explicit NodeInfoImpl(std::function<ppctars::NodeInfo*()> inner) : m_inner(std::move(inner)) {}

    NodeInfoImpl(bcos::bytesConstRef const& nodeID)
      : m_inner([inner = ppctars::NodeInfo()]() mutable { return &inner; })
    {
        m_inner()->nodeID = std::vector<char>(nodeID.begin(), nodeID.end());
    }
    NodeInfoImpl(bcos::bytesConstRef const& nodeID, std::string const& endPoint)
      : NodeInfoImpl(nodeID)
    {
        m_inner()->endPoint = endPoint;
    }
    ~NodeInfoImpl() override = default;

    void setComponents(std::vector<std::string> const& components) override
    {
        m_components = std::set<std::string>(components.begin(), components.end());
        m_inner()->components = components;
    }
    std::set<std::string> const& components() const override { return m_components; }

    std::string const& endPoint() const override { return m_inner()->endPoint; }

    bcos::bytesConstRef nodeID() const override
    {
        return {reinterpret_cast<const bcos::byte*>(m_inner()->nodeID.data()),
            m_inner()->nodeID.size()};
    }

    void encode(bcos::bytes& data) const override;
    void decode(bcos::bytesConstRef data) override;
    ppctars::NodeInfo const& inner() { return *(m_inner()); }

    void setFront(ppc::front::IFront::Ptr&& front) override { m_front = std::move(front); }
    ppc::front::IFront::Ptr const& getFront() const override { return m_front; }

private:
    ppc::front::IFront::Ptr m_front;
    std::set<std::string> m_components;
    std::function<ppctars::NodeInfo*()> m_inner;
};

class NodeInfoFactory : public INodeInfoFactory
{
public:
    using Ptr = std::shared_ptr<NodeInfoFactory>;
    NodeInfoFactory(bcos::bytesConstRef const& nodeID) : INodeInfoFactory(nodeID.toBytes()) {}
    ~NodeInfoFactory() override {}

    INodeInfo::Ptr build() override { return std::make_shared<NodeInfoImpl>(bcos::ref(m_nodeID)); }


    INodeInfo::Ptr build(std::string const& endPoint) override
    {
        return std::make_shared<NodeInfoImpl>(bcos::ref(m_nodeID), endPoint);
    }
};
}  // namespace ppc::protocol