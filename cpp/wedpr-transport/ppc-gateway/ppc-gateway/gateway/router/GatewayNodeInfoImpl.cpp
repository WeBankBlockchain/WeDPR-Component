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
#include "ppc-gateway/Common.h"
#include "wedpr-protocol/protobuf/src/Common.h"
#include "wedpr-protocol/protobuf/src/NodeInfoImpl.h"
#include "wedpr-protocol/tars/Common.h"

using namespace ppctars;
using namespace ppc::protocol;
using namespace ppc::gateway;


// the gateway nodeID
std::string const& GatewayNodeInfoImpl::p2pNodeID() const
{
    return m_rawGatewayInfo->p2pnodeid();
}
// the agency
std::string const& GatewayNodeInfoImpl::agency() const
{
    return m_rawGatewayInfo->agency();
}

uint32_t GatewayNodeInfoImpl::statusSeq() const
{
    return m_rawGatewayInfo->statusseq();
}
void GatewayNodeInfoImpl::setStatusSeq(uint32_t statusSeq)
{
    m_rawGatewayInfo->set_statusseq(statusSeq);
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

bool GatewayNodeInfoImpl::existComponent(std::string const& component) const
{
    bcos::ReadGuard l(x_nodeList);
    for (auto const& it : m_nodeList)
    {
        if (it.second->componentExist(component))
        {
            return true;
        }
    }
    return false;
}

void GatewayNodeInfoImpl::updateNodeList()
{
    // Note: can't use clear_nodelist here, for clear_nodelist will destroy the allocated nodelist,
    // and cause double release coredump
    releaseWithoutDestory();
    // re-encode nodeList
    for (auto const& it : m_nodeList)
    {
        auto nodeInfo = std::dynamic_pointer_cast<NodeInfoImpl>(it.second);
        nodeInfo->encodeFields();
        m_rawGatewayInfo->mutable_nodelist()->UnsafeArenaAddAllocated(
            nodeInfo->rawNodeInfo().get());
    }
}

// Note: this is wrappered with lock
bool GatewayNodeInfoImpl::tryAddNodeInfo(INodeInfo::Ptr const& info)
{
    auto nodeID = info->nodeID().toBytes();
    auto existedNodeInfo = nodeInfo(nodeID);
    // the node info has not been updated
    if (existedNodeInfo != nullptr && existedNodeInfo->equal(info))
    {
        auto meta = info->meta();
        // update the meta
        if (meta != existedNodeInfo->meta())
        {
            existedNodeInfo->setMeta(meta);
            GATEWAY_LOG(INFO) << LOG_DESC("tryAddNodeInfo, update the meta, updated nodeInfo")
                              << printNodeInfo(existedNodeInfo);
        }
        // update the components
        auto components = info->copiedComponents();
        if (components != existedNodeInfo->copiedComponents())
        {
            existedNodeInfo->setComponents(
                std::set<std::string>(components.begin(), components.end()));
            GATEWAY_LOG(INFO) << LOG_DESC("tryAddNodeInfo, update the components, updated nodeInfo")
                              << printNodeInfo(existedNodeInfo);
        }
        return false;
    }
    {
        bcos::WriteGuard l(x_nodeList);
        m_nodeList[nodeID] = info;
        updateNodeList();
    }
    return true;
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
        updateNodeList();
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
    bool selectAll, bcos::bytes const& fromNode, std::string const& topic) const
{
    std::vector<std::shared_ptr<ppc::front::IFrontClient>> result;
    // empty topic means broadcast message to all front
    if (topic.empty())
    {
        bcos::ReadGuard l(x_nodeList);
        for (auto const& it : m_nodeList)
        {
            result.emplace_back(it.second->getFront());
        }
        return result;
    }
    // the topic specified
    bcos::ReadGuard l(x_topicInfo);
    for (auto const& it : m_topicInfo)
    {
        INodeInfo::Ptr selectedNode = nullptr;
        if (it.second.count(topic))
        {
            selectedNode = nodeInfo(it.first);
        }
        // ignore the fromNode
        if (selectedNode != nullptr && selectedNode->nodeID().toBytes() != fromNode)
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
    encodePBObject(data, m_rawGatewayInfo);
}

void GatewayNodeInfoImpl::decode(bcos::bytesConstRef data)
{
    decodePBObject(m_rawGatewayInfo, data);
    {
        bcos::WriteGuard l(x_nodeList);
        // decode into m_nodeList
        m_nodeList.clear();
        for (int i = 0; i < m_rawGatewayInfo->nodelist_size(); i++)
        {
            std::shared_ptr<ppc::proto::NodeInfo> rawNodeInfo(
                m_rawGatewayInfo->mutable_nodelist(i));
            auto nodeInfoPtr = std::make_shared<NodeInfoImpl>(rawNodeInfo);
            m_nodeList.insert(std::make_pair(nodeInfoPtr->nodeID().toBytes(), nodeInfoPtr));
        }
    }
}

void GatewayNodeInfoImpl::toJson(Json::Value& jsonObject) const
{
    bcos::ReadGuard l(x_nodeList);
    jsonObject["gatewayNodeID"] = p2pNodeID();
    jsonObject["agency"] = agency();

    auto agencyNodeList = nodeList();
    Json::Value frontList(Json::arrayValue);
    for (auto const& it : agencyNodeList)
    {
        Json::Value nodeInfo;
        it.second->toJson(nodeInfo);
        frontList.append(nodeInfo);
    }
    jsonObject["frontList"] = frontList;
}