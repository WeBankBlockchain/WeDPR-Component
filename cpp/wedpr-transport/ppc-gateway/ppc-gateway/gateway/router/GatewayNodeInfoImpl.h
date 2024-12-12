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
#include "NodeInfo.pb.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::gateway
{
class GatewayNodeInfoImpl : public GatewayNodeInfo
{
public:
    using Ptr = std::shared_ptr<GatewayNodeInfoImpl>;
    GatewayNodeInfoImpl() : m_rawGatewayInfo(std::make_shared<ppc::proto::GatewayNodeInfo>()) {}
    GatewayNodeInfoImpl(std::string const& p2pNodeID) : GatewayNodeInfoImpl()
    {
        m_rawGatewayInfo->set_p2pnodeid(p2pNodeID);
    }
    GatewayNodeInfoImpl(std::string const& p2pNodeID, std::string const& agency)
      : GatewayNodeInfoImpl(p2pNodeID)
    {
        m_rawGatewayInfo->set_agency(agency);
    }

    GatewayNodeInfoImpl(bcos::bytesConstRef data) : GatewayNodeInfoImpl() { decode(data); }

    ~GatewayNodeInfoImpl() override { releaseWithoutDestory(); }

    // the gateway nodeID
    std::string const& p2pNodeID() const override;
    // the agency
    std::string const& agency() const override;
    // the node information

    // get the node information by nodeID
    ppc::protocol::INodeInfo::Ptr nodeInfo(bcos::bytes const& nodeID) const override;

    void encode(bcos::bytes& data) const override;
    void decode(bcos::bytesConstRef data) override;

    bool tryAddNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo, bool& updated) override;
    void removeNodeInfo(bcos::bytes const& nodeID) override;

    std::vector<std::shared_ptr<ppc::front::IFrontClient>> chooseRouteByComponent(
        bool selectAll, std::string const& component) const override;
    std::vector<std::shared_ptr<ppc::front::IFrontClient>> chooseRouterByAgency(
        bool selectAll) const override;
    std::vector<std::shared_ptr<ppc::front::IFrontClient>> chooseRouterByTopic(
        bool selectAll, bcos::bytes const& fromNode, std::string const& topic) const override;

    void registerTopic(bcos::bytes const& nodeID, std::string const& topic) override;
    void unRegisterTopic(bcos::bytes const& nodeID, std::string const& topic) override;

    std::map<bcos::bytes, ppc::protocol::INodeInfo::Ptr> nodeList() const override
    {
        bcos::ReadGuard l(x_nodeList);
        return m_nodeList;
    }
    uint32_t statusSeq() const override;
    void setStatusSeq(uint32_t statusSeq) override;

    virtual uint16_t nodeSize() const override
    {
        bcos::ReadGuard l(x_nodeList);
        return m_nodeList.size();
    }

    void toJson(Json::Value& jsonObject) const override;

    bool existComponent(std::string const& component) const override;

private:
    void updateNodeList();

    void releaseWithoutDestory()
    {
        // return back the ownership to nodeList to shared_ptr
        auto allocatedNodeListSize = m_rawGatewayInfo->nodelist_size();
        for (int i = 0; i < allocatedNodeListSize; i++)
        {
            m_rawGatewayInfo->mutable_nodelist()->UnsafeArenaReleaseLast();
        }
    }

private:
    std::shared_ptr<ppc::proto::GatewayNodeInfo> m_rawGatewayInfo;
    // NodeID => nodeInfo
    std::map<bcos::bytes, ppc::protocol::INodeInfo::Ptr> m_nodeList;
    mutable bcos::SharedMutex x_nodeList;

    // NodeID=>topics(Note serialized)
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

    GatewayNodeInfo::Ptr build(std::string const& p2pNode) const override
    {
        return std::make_shared<GatewayNodeInfoImpl>(p2pNode);
    }

private:
    std::string m_p2pNodeID;
    std::string m_agency;
};
}  // namespace ppc::gateway