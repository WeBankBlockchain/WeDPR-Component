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
 * @file GatewayNodeInfoImpl.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */
#include "GatewayNodeInfoImpl.h"
#include "wedpr-protocol/protobuf/Common.h"
#include "wedpr-protocol/protobuf/NodeInfoImpl.h"
#include "wedpr-protocol/tars/Common.h"

using namespace ppctars;
using namespace ppc::protocol;
using namespace ppc::gateway;


// the gateway nodeID
std::string const& GatewayNodeInfoImpl::p2pNodeID() const
{
    return m_inner()->p2pnodeid();
}
// the agency
std::string const& GatewayNodeInfoImpl::agency() const
{
    return m_inner()->agency();
}

uint32_t GatewayNodeInfoImpl::statusSeq() const
{
    return m_inner()->statusseq();
}
void GatewayNodeInfoImpl::setStatusSeq(uint32_t statusSeq)
{
    m_inner()->set_statusseq(statusSeq);
}

// get the node information by nodeID
INodeInfo::Ptr GatewayNodeInfoImpl::nodeInfo(bcos::bytes const& nodeID) const
{
    bcos::ReadGuard l(x_nodeList);
    if (m_nodeList.count(nodeID))
    {
        return m_nodeList.at(nodeID);
    }
    return nullptr;
}

bool GatewayNodeInfoImpl::tryAddNodeInfo(INodeInfo::Ptr const& info)
{
    auto nodeID = info->nodeID().toBytes();
    auto existedNodeInfo = nodeInfo(nodeID);
    // update the info
    if (existedNodeInfo == nullptr || !existedNodeInfo->equal(info))
    {
        bcos::WriteGuard l(x_nodeList);
        m_nodeList[nodeID] = info;
        return true;
    }
    return false;
}

void GatewayNodeInfoImpl::removeNodeInfo(bcos::bytes const& nodeID)
{
    // remove the nodeInfo
    {
        bcos::UpgradableGuard l(x_nodeList);
        auto it = m_nodeList.find(nodeID);
        if (it == m_nodeList.end())
        {
            return;
        }
        bcos::UpgradeGuard ul(l);
        m_nodeList.erase(it);
    }
    // remove the topic info
    {
        bcos::UpgradableGuard l(x_topicInfo);
        auto it = m_topicInfo.find(nodeID);
        if (it != m_topicInfo.end())
        {
            bcos::UpgradeGuard ul(l);
            m_topicInfo.erase(it);
        }
    }
}

std::vector<std::shared_ptr<ppc::front::IFrontClient>> GatewayNodeInfoImpl::chooseRouteByComponent(
    bool selectAll, std::string const& component) const
{
    std::vector<std::shared_ptr<ppc::front::IFrontClient>> result;
    bcos::ReadGuard l(x_nodeList);
    for (auto const& it : m_nodeList)
    {
        if (it.second->components().count(component))
        {
            result.emplace_back(it.second->getFront());
        }
        if (!result.empty() && !selectAll)
        {
            break;
        }
    }
    return result;
}


std::vector<std::shared_ptr<ppc::front::IFrontClient>> GatewayNodeInfoImpl::chooseRouterByAgency(
    bool selectAll) const
{
    std::vector<std::shared_ptr<ppc::front::IFrontClient>> result;
    bcos::ReadGuard l(x_nodeList);
    for (auto const& it : m_nodeList)
    {
        result.emplace_back(it.second->getFront());
        if (!result.empty() && !selectAll)
        {
            break;
        }
    }
    return result;
}

std::vector<std::shared_ptr<ppc::front::IFrontClient>> GatewayNodeInfoImpl::chooseRouterByTopic(
    bool selectAll, std::string const& topic) const
{
    std::vector<std::shared_ptr<ppc::front::IFrontClient>> result;
    bcos::ReadGuard l(x_topicInfo);
    for (auto const& it : m_topicInfo)
    {
        INodeInfo::Ptr selectedNode = nullptr;
        if (it.second.count(topic))
        {
            selectedNode = nodeInfo(it.first);
        }
        if (selectedNode != nullptr)
        {
            result.emplace_back(selectedNode->getFront());
        }
        if (!result.empty() && !selectAll)
        {
            break;
        }
    }
    return result;
}
void GatewayNodeInfoImpl::registerTopic(bcos::bytes const& nodeID, std::string const& topic)
{
    bcos::UpgradableGuard l(x_topicInfo);
    if (m_topicInfo.count(nodeID) && m_topicInfo.at(nodeID).count(topic))
    {
        return;
    }
    bcos::UpgradeGuard ul(l);
    if (!m_topicInfo.count(nodeID))
    {
        m_topicInfo[nodeID] = std::set<std::string>();
    }
    m_topicInfo[nodeID].insert(topic);
}

void GatewayNodeInfoImpl::unRegisterTopic(bcos::bytes const& nodeID, std::string const& topic)
{
    bcos::UpgradableGuard l(x_topicInfo);
    if (!m_topicInfo.count(nodeID) || !m_topicInfo.at(nodeID).count(topic))
    {
        return;
    }
    bcos::UpgradeGuard ul(l);
    m_topicInfo[nodeID].erase(topic);
}

void GatewayNodeInfoImpl::encode(bcos::bytes& data) const
{
    m_inner()->clear_nodelist();
    {
        bcos::ReadGuard l(x_nodeList);
        // encode nodeList
        for (auto const& it : m_nodeList)
        {
            auto nodeInfo = std::dynamic_pointer_cast<NodeInfoImpl>(it.second);
            m_inner()->mutable_nodelist()->UnsafeArenaAddAllocated(nodeInfo->innerFunc()());
        }
    }
    encodePBObject(data, m_inner());
}

void GatewayNodeInfoImpl::decode(bcos::bytesConstRef data)
{
    decodePBObject(m_inner(), data);
    {
        bcos::WriteGuard l(x_nodeList);
        // decode into m_nodeList
        m_nodeList.clear();
        for (int i = 0; i < m_inner()->nodelist_size(); i++)
        {
            auto nodeInfoPtr = std::make_shared<NodeInfoImpl>(
                [m_entry = m_inner()->nodelist(i)]() mutable { return &m_entry; });
            m_nodeList.insert(std::make_pair(nodeInfoPtr->nodeID().toBytes(), nodeInfoPtr));
        }
    }
}