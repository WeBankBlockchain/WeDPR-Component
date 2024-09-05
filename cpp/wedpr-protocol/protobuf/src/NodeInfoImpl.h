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
#include "NodeInfo.pb.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include <memory>

namespace ppc::protocol
{
// the node information
class NodeInfoImpl : public INodeInfo
{
public:
    using Ptr = std::shared_ptr<NodeInfoImpl>;
    NodeInfoImpl() { m_rawNodeInfo = std::make_shared<ppc::proto::NodeInfo>(); }
    explicit NodeInfoImpl(std::shared_ptr<ppc::proto::NodeInfo> rawNodeInfo)
      : m_rawNodeInfo(rawNodeInfo)
    {}
    NodeInfoImpl(bcos::bytesConstRef const& data) : NodeInfoImpl() { decode(data); }

    NodeInfoImpl(bcos::bytesConstRef const& nodeID, std::string const& endPoint) : NodeInfoImpl()
    {
        m_rawNodeInfo->set_nodeid(nodeID.data(), nodeID.size());
        m_rawNodeInfo->set_endpoint(endPoint);
    }

    ~NodeInfoImpl() override {}

    void setNodeID(bcos::bytesConstRef nodeID) override
    {
        m_rawNodeInfo->set_nodeid(nodeID.data(), nodeID.size());
    }
    void setEndPoint(std::string const& endPoint) override
    {
        m_rawNodeInfo->set_endpoint(endPoint);
    }

    void setComponents(std::set<std::string> const& components) override
    {
        m_components = components;
    }
    std::set<std::string> const& components() const override { return m_components; }

    std::string const& endPoint() const override { return m_rawNodeInfo->endpoint(); }

    bcos::bytesConstRef nodeID() const override
    {
        return {reinterpret_cast<const bcos::byte*>(m_rawNodeInfo->nodeid().data()),
            m_rawNodeInfo->nodeid().size()};
    }

    void encode(bcos::bytes& data) const override;
    void decode(bcos::bytesConstRef data) override;
    std::shared_ptr<ppc::proto::NodeInfo> rawNodeInfo() { return m_rawNodeInfo; }

    void setFront(std::shared_ptr<ppc::front::IFrontClient>&& front) override
    {
        m_front = std::move(front);
    }
    std::shared_ptr<ppc::front::IFrontClient> const& getFront() const override { return m_front; }

private:
    std::shared_ptr<ppc::front::IFrontClient> m_front;
    std::set<std::string> m_components;
    std::shared_ptr<ppc::proto::NodeInfo> m_rawNodeInfo;
};

class NodeInfoFactory : public INodeInfoFactory
{
public:
    using Ptr = std::shared_ptr<NodeInfoFactory>;
    NodeInfoFactory() {}
    ~NodeInfoFactory() override {}

    INodeInfo::Ptr build() override { return std::make_shared<NodeInfoImpl>(); }

    INodeInfo::Ptr build(bcos::bytesConstRef data) override
    {
        return std::make_shared<NodeInfoImpl>(data);
    }
    INodeInfo::Ptr build(bcos::bytesConstRef nodeID, std::string const& endPoint) override
    {
        return std::make_shared<NodeInfoImpl>(nodeID, endPoint);
    }
};
}  // namespace ppc::protocol