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
 * @file TransportBuilder.cpp
 * @author: yujiechen
 * @date 2024-09-04
 */

#include "TransportBuilder.h"
#include "ProTransportImpl.h"
#include "Transport.h"
#include "TransportImpl.h"
#include "ppc-front/FrontConfigImpl.h"
#include "protobuf/src/NodeInfoImpl.h"
#include <memory>

using namespace ppc::sdk;
using namespace ppc::front;

TransportBuilder::TransportBuilder()
{
    m_frontConfigBuilder = std::make_shared<FrontConfigBuilderImpl>(
        std::make_shared<ppc::protocol::NodeInfoFactory>());
}

Transport::Ptr TransportBuilder::build(
    SDKMode mode, ppc::front::FrontConfig::Ptr config, ppc::gateway::IGateway::Ptr gateway)
{
    switch (mode)
    {
    case SDKMode::AIR:
    {
        return std::make_shared<TransportImpl>(config, gateway);
    }
    case SDKMode::PRO:
    {
        return std::make_shared<ProTransportImpl>(config);
    }
    default:
        throw std::runtime_error("Unsupported sdk mode, only support AIR/PRO mode!");
    }
}

ppc::front::FrontConfig::Ptr TransportBuilder::buildConfig(int threadPoolSize, std::string nodeID)
{
    return m_frontConfigBuilder->build(threadPoolSize, nodeID);
}


void TransportBuilder::initLog(const std::string& configPath)
{
    bcos::Guard l(x_logInited);
    if (m_logInited)
    {
        return;
    }
    m_logInited = true;
    // init the log
    boost::property_tree::ptree pt;
    try
    {
        boost::property_tree::read_ini(configPath, pt);
    }
    catch (std::exception const& e)
    {
        // disable the log when the configPath not exists
        pt.put("log.enable", false);
    }
    m_logInitializer = std::make_shared<bcos::BoostLogInitializer>();
    m_logInitializer->initLog(pt, bcos::FileLogger, "gateway_sdk_log");
}