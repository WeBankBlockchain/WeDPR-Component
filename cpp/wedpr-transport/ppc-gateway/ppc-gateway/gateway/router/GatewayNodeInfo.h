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
 * @file GatewayNodeInfo.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-framework/Helper.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include "ppc-utilities/Utilities.h"
#include <bcos-utilities/Log.h>
#include <memory>
#include <sstream>
namespace ppc::gateway
{
class GatewayNodeInfo
{
public:
    using Ptr = std::shared_ptr<GatewayNodeInfo>;
    GatewayNodeInfo() = default;
    virtual ~GatewayNodeInfo() = default;

    // the gateway nodeID
    virtual std::string const& p2pNodeID() const = 0;
    // the agency
    virtual std::string const& agency() const = 0;
    virtual uint32_t statusSeq() const = 0;
    virtual void setStatusSeq(uint32_t statusSeq) = 0;

    // get the node information by nodeID
    virtual ppc::protocol::INodeInfo::Ptr nodeInfo(bcos::bytes const& nodeID) const = 0;
    virtual bool tryAddNodeInfo(ppc::protocol::INodeInfo::Ptr const& nodeInfo, bool& updated) = 0;
    virtual void removeNodeInfo(bcos::bytes const& nodeID) = 0;

    virtual std::vector<std::shared_ptr<ppc::front::IFrontClient>> chooseRouteByComponent(
        bool selectAll, std::string const& component) const = 0;
    virtual std::vector<std::shared_ptr<ppc::front::IFrontClient>> chooseRouterByAgency(
        bool selectAll) const = 0;
    virtual std::vector<std::shared_ptr<ppc::front::IFrontClient>> chooseRouterByTopic(
        bool selectAll, bcos::bytes const& fromNode, std::string const& topic) const = 0;

    virtual void encode(bcos::bytes& data) const = 0;
    virtual void decode(bcos::bytesConstRef data) = 0;

    virtual void registerTopic(bcos::bytes const& nodeID, std::string const& topic) = 0;
    virtual void unRegisterTopic(bcos::bytes const& nodeID, std::string const& topic) = 0;

    virtual std::map<bcos::bytes, ppc::protocol::INodeInfo::Ptr> nodeList() const = 0;
    virtual bool existComponent(std::string const& component) const = 0;
    virtual uint16_t nodeSize() const = 0;
    virtual void toJson(Json::Value& jsonObject) const = 0;
};

class GatewayNodeInfoFactory
{
public:
    using Ptr = std::shared_ptr<GatewayNodeInfoFactory>;
    GatewayNodeInfoFactory() = default;
    virtual ~GatewayNodeInfoFactory() = default;

    virtual GatewayNodeInfo::Ptr build() const = 0;
    virtual GatewayNodeInfo::Ptr build(std::string const& p2pNode) const = 0;
};
struct GatewayNodeInfoCmp
{
    bool operator()(GatewayNodeInfo::Ptr const& _first, GatewayNodeInfo::Ptr const& _second) const
    {
        // increase order
        return _first->p2pNodeID() > _second->p2pNodeID();
    }
};
using GatewayNodeInfos = std::set<GatewayNodeInfo::Ptr, GatewayNodeInfoCmp>;

inline std::string printNodeStatus(GatewayNodeInfo::Ptr const& status)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("p2pNodeID", printP2PIDElegantly(status->p2pNodeID()))
                 << LOG_KV("agency", status->agency()) << LOG_KV("statusSeq", status->statusSeq())
                 << LOG_KV("nodeSize", status->nodeSize());
    auto nodeInfoList = status->nodeList();
    for (auto const& it : nodeInfoList)
    {
        stringstream << printNodeInfo(it.second);
    }
    return stringstream.str();
}
}  // namespace ppc::gateway