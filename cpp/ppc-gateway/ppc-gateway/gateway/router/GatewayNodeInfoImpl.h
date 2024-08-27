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
 * @file GatewayNodeInfoImpl.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "GatewayNodeInfo.h"
#include "ppc-tars-protocol/tars/NodeInfo.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::gateway
{
class GatewayNodeInfoImpl : public GatewayNodeInfo
{
public:
    using Ptr = std::shared_ptr<GatewayNodeInfoImpl>;
    GatewayNodeInfoImpl(std::string const& p2pNodeID, std::string const& agency)
      : m_inner([inner = ppctars::GatewayNodeInfo()]() mutable { return &inner; })
    {
        m_inner()->p2pNodeID = p2pNodeID;
        m_inner()->agency = agency;
    }
    ~GatewayNodeInfoImpl() override = default;

    // the gateway nodeID
    std::string const& p2pNodeID() const override;
    // the agency
    std::string const& agency() const override;
    // the node information

    // get the node information by nodeID
    ppc::protocol::INodeInfo::Ptr nodeInfo(bcos::bytes const& nodeID) const override;

    void encode(bcos::bytes& data) const override;
    void decode(bcos::bytesConstRef data) override;

    bool tryAddNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo) override;
    void removeNodeInfo(bcos::bytes const& nodeID) override;

    std::vector<ppc::front::IFront::Ptr> chooseRouteByComponent(
        bool selectAll, std::string const& component) const override;
    std::vector<ppc::front::IFront::Ptr> chooseRouterByAgency(bool selectAll) const override;
    std::vector<ppc::front::IFront::Ptr> chooseRouterByTopic(
        bool selectAll, std::string const& topic) const override;

    void registerTopic(bcos::bytes const& nodeID, std::string const& topic) override;
    void unRegisterTopic(bcos::bytes const& nodeID, std::string const& topic) override;

    std::map<bcos::bytes, ppc::protocol::INodeInfo::Ptr> nodeList() const override
    {
        bcos::WriteGuard l(x_nodeList);
        return m_nodeList;
    }

private:
    std::function<ppctars::GatewayNodeInfo*()> m_inner;
    // NodeID => nodeInfo
    std::map<bcos::bytes, ppc::protocol::INodeInfo::Ptr> m_nodeList;
    mutable bcos::SharedMutex x_nodeList;

    // NodeID=>topics
    using Topics = std::set<std::string>;
    std::map<bcos::bytes, Topics> m_topicInfo;
    mutable bcos::SharedMutex x_topicInfo;
};

class GatewayNodeInfoFactoryImpl : public GatewayNodeInfoFactory
{
public:
    using Ptr = std::shared_ptr<GatewayNodeInfoFactoryImpl>;
    GatewayNodeInfoFactoryImpl(std::string const& p2pNodeID, std::string const& agency)
      : m_p2pNodeID(p2pNodeID), m_agency(agency)
    {}
    ~GatewayNodeInfoFactoryImpl() override = default;

    GatewayNodeInfo::Ptr build() const override
    {
        return std::make_shared<GatewayNodeInfoImpl>(m_p2pNodeID, m_agency);
    }

private:
    std::string m_p2pNodeID;
    std::string m_agency;
};
}  // namespace ppc::gateway