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

#include "NodeInfoImpl.h"
#include "Common.h"

using namespace ppc::protocol;

void NodeInfoImpl::encode(bcos::bytes& data) const
{
    encodeFields();
    encodePBObject(data, m_rawNodeInfo);
}

void NodeInfoImpl::encodeFields() const
{
    bcos::ReadGuard l(x_components);
    // set the components
    for (auto const& component : m_components)
    {
        m_rawNodeInfo->add_components(component);
    }
}
void NodeInfoImpl::decode(bcos::bytesConstRef data)
{
    decodePBObject(m_rawNodeInfo, data);
    decodeFields();
}

void NodeInfoImpl::decodeFields()
{
    bcos::WriteGuard l(x_components);
    m_components = std::set<std::string>(
        m_rawNodeInfo->components().begin(), m_rawNodeInfo->components().end());
}

void NodeInfoImpl::toJson(Json::Value& jsonObject) const
{
    jsonObject["nodeID"] = std::string(nodeID().begin(), nodeID().end());
    jsonObject["endPoint"] = endPoint();
    Json::Value componentsInfo(Json::arrayValue);
    auto componentsList = components();
    for (auto const& it : componentsList)
    {
        componentsInfo.append(it);
    }
    jsonObject["components"] = componentsInfo;
    jsonObject["meta"] = meta();
}