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
#include "ppc-framework/front/IFront.h"
#include <memory>

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

    // components
    virtual void setComponents(std::vector<std::string> const& components) = 0;
    virtual std::set<std::string> const& components() const = 0;

    virtual void encode(bcos::bytes& data) const = 0;
    virtual void decode(bcos::bytesConstRef data) = 0;

    virtual void setFront(ppc::front::IFront::Ptr&& front) = 0;
    virtual ppc::front::IFront::Ptr const& getFront() const = 0;

    virtual bool equal(INodeInfo::Ptr const& info)
    {
        return (nodeID() == info->nodeID()) && (components() == info->components());
    }
};
class INodeInfoFactory
{
public:
    using Ptr = std::shared_ptr<INodeInfoFactory>;
    INodeInfoFactory(bcos::bytes nodeID) : m_nodeID(std::move(nodeID)) {}
    virtual ~INodeInfoFactory() = default;

    virtual INodeInfo::Ptr build() = 0;
    virtual INodeInfo::Ptr build(std::string const& endPoint) = 0;

protected:
    bcos::bytes m_nodeID;
};
}  // namespace ppc::protocol