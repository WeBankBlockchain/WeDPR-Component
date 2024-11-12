/*
 *  Copyright (C) 2022 WeDPR.
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
 * @file ServiceConfig.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "ServiceConfig.h"
#include "ppc-framework/Common.h"

using namespace ppc::protocol;


void EntryPointInfo::encode(Json::Value& json) const
{
    json["serviceName"] = m_serviceName;
    json["entryPoint"] = m_entryPoint;
}

void EntryPointInfo::decode(Json::Value const& jsonObject)
{
    if (jsonObject.isMember("serviceName"))
    {
        m_serviceName = jsonObject["serviceName"].asString();
    }
    if (jsonObject.isMember("entryPoint"))
    {
        m_entryPoint = jsonObject["entryPoint"].asString();
    }
}


void ServiceConfig::addEntryPoint(EntryPointInfo entryPoint)
{
    m_serviceInfos.emplace_back(std::move(entryPoint));
}

// encode the serviceConfig
std::string ServiceConfig::encode() const
{
    Json::Value result;
    Json::Value serviceInfos(Json::arrayValue);
    for (auto const& it : m_serviceInfos)
    {
        Json::Value entryPointInfo;
        it.encode(entryPointInfo);
        serviceInfos.append(entryPointInfo);
    }
    result["serviceInfos"] = serviceInfos;
    Json::FastWriter fastWriter;
    return fastWriter.write(result);
}

// decode the serviceConfig
void ServiceConfig::decode(std::string const& data)
{
    Json::Value root;
    Json::Reader jsonReader;
    if (!jsonReader.parse(std::string(data), root))
    {
        BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                  "The task information must be valid json string."));
    }
    if (!root.isMember("serviceInfos"))
    {
        return;
    }
    for (const auto& serviceInfo : root["serviceInfos"])
    {
        m_serviceInfos.emplace_back(EntryPointInfo(serviceInfo));
    }
}