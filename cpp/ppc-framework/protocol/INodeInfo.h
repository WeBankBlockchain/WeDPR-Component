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
 * @file INodeInfo.h
 * @author: yujiechen
 * @date 2024-08-26
 */
#pragma once
#include "ppc-framework/Helper.h"
#include <bcos-utilities/Common.h>
#include <json/json.h>
#include <memory>
#include <set>
#include <sstream>
#include <string>

namespace ppc::front
{
class IFrontClient;
}
namespace ppc::protocol
{
// the node information
class INodeInfo
{
public:
    using Ptr = std::shared_ptr<INodeInfo>;
    INodeInfo() = default;
    virtual ~INodeInfo() = default;

    virtual std::string const& endPoint() const = 0;
    virtual bcos::bytesConstRef nodeID() const = 0;
    virtual void setNodeID(bcos::bytesConstRef nodeID) = 0;
    virtual void setEndPoint(std::string const& endPoint) = 0;

    // components
    virtual void setComponents(std::set<std::string> const& components) = 0;
    virtual bool addComponent(std::string const& component) = 0;
    virtual bool eraseComponent(std::string const& component) = 0;
    virtual bool componentExist(std::string const& component) const = 0;
    virtual std::set<std::string> const& components() const = 0;
    virtual std::vector<std::string> copiedComponents() const = 0;

    virtual void encode(bcos::bytes& data) const = 0;
    virtual void decode(bcos::bytesConstRef data) = 0;

    virtual void setFront(std::shared_ptr<ppc::front::IFrontClient>&& front) = 0;
    virtual std::shared_ptr<ppc::front::IFrontClient> const& getFront() const = 0;

    virtual bool equal(INodeInfo::Ptr const& info)
    {
        return (nodeID().toBytes() == info->nodeID().toBytes()) && (endPoint() == info->endPoint());
    }

    virtual std::string meta() const = 0;
    // the node meta information
    virtual void setMeta(std::string const& meta) = 0;
    virtual void toJson(Json::Value& jsonObject) const = 0;
};
class INodeInfoFactory
{
public:
    using Ptr = std::shared_ptr<INodeInfoFactory>;
    INodeInfoFactory() = default;
    virtual ~INodeInfoFactory() = default;

    virtual INodeInfo::Ptr build() = 0;
    virtual INodeInfo::Ptr build(bcos::bytesConstRef nodeID, std::string const& endPoint) = 0;
    virtual INodeInfo::Ptr build(bcos::bytesConstRef data) = 0;
};

inline std::string printNodeInfo(INodeInfo::Ptr const& nodeInfo)
{
    if (!nodeInfo)
    {
        return "nullptr";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("endPoint", nodeInfo->endPoint())
                 << LOG_KV("nodeID", printNodeID(nodeInfo->nodeID()));
    std::string components = "";
    for (auto const& it : nodeInfo->copiedComponents())
    {
        components = components + it + ",";
    }
    stringstream << LOG_KV("components", components) << LOG_KV("meta", nodeInfo->meta());
    return stringstream.str();
}
}  // namespace ppc::protocol