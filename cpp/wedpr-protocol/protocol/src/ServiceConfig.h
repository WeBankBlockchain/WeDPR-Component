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
 * @file ServiceConfig.h
 * @author: yujiechen
 * @date 2022-11-4
 */
#pragma once
#include <json/json.h>
#include <memory>
#include <string>
#include <vector>

namespace ppc::protocol
{
class EntryPointInfo
{
public:
    EntryPointInfo(std::string const& serviceName, std::string const& entryPoint)
      : m_serviceName(serviceName), m_entryPoint(entryPoint)
    {}

    EntryPointInfo(Json::Value const& jsonObject) { decode(jsonObject); }

    virtual ~EntryPointInfo() = default;

    virtual void encode(Json::Value& json) const;

    virtual void decode(Json::Value const& jsonObject);

    std::string const& serviceName() const { return m_serviceName; }
    std::string const& entryPoint() const { return m_entryPoint; }

private:
    std::string m_serviceName;
    std::string m_entryPoint;
};

class ServiceConfig
{
public:
    using Ptr = std::shared_ptr<ServiceConfig>;
    ServiceConfig() = default;
    ServiceConfig(std::string const& data) { decode(data); }
    virtual ~ServiceConfig() = default;

    void addEntryPoint(EntryPointInfo entryPoint);

    // encode the serviceConfig
    virtual std::string encode() const;

    // decode the serviceConfig
    virtual void decode(std::string const& data);
    std::vector<EntryPointInfo> const& serviceInfos() const { return m_serviceInfos; }

private:
    std::vector<EntryPointInfo> m_serviceInfos;
};

class ServiceConfigBuilder
{
public:
    using Ptr = std::shared_ptr<ServiceConfigBuilder>;
    ServiceConfigBuilder() = default;
    virtual ~ServiceConfigBuilder() = default;

    EntryPointInfo buildEntryPoint(std::string const& serviceName, std::string const& entryPoint)
    {
        return EntryPointInfo(serviceName, entryPoint);
    }

    ServiceConfig buildServiceConfig() const { return ServiceConfig(); }
    ServiceConfig populateServiceConfig(std::string const& data) { return ServiceConfig(data); }
};
}  // namespace ppc::protocol