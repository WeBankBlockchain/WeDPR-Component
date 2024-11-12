/**
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
 * @file ProNodeInitializer.cpp
 * @author: yujiechen
 * @date 2022-11-14
 */
#include "ProNodeInitializer.h"
#include "ppc-rpc/src/RpcFactory.h"
#include "ppc-rpc/src/RpcMemory.h"
#include "wedpr-protocol/grpc/client/RemoteFrontBuilder.h"

using namespace ppc::protocol;
using namespace ppc::node;
using namespace ppc::gateway;
using namespace ppc::rpc;
using namespace ppc::storage;
using namespace ppc::initializer;
using namespace bcos;

ProNodeInitializer::ProNodeInitializer() {}

void ProNodeInitializer::init(std::string const& _configPath)
{
    // init the log
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(_configPath, pt);

    m_logInitializer = std::make_shared<BoostLogInitializer>();
    m_logInitializer->initLog(pt);
    INIT_LOG(INFO) << LOG_DESC("initLog success");

    // init the node
    m_nodeInitializer = std::make_shared<Initializer>(ppc::protocol::NodeArch::PRO, _configPath);
    // load the rpc config
    m_nodeInitializer->config()->loadRpcConfig(pt);

    // init the node(no need to set the gateway)
    m_nodeInitializer->init(nullptr);


    INIT_LOG(INFO) << LOG_DESC("init the rpc");
    // init RpcStatusInterface
    RpcStatusInterface::Ptr rpcStatusInterface = std::make_shared<ppc::rpc::RpcMemory>();


    auto rpcFactory = std::make_shared<RpcFactory>(m_nodeInitializer->config()->agencyID());
    m_rpc = rpcFactory->buildRpc(
        m_nodeInitializer->config(), m_nodeInitializer->transport()->gateway());
    m_rpc->setRpcStorage(rpcStatusInterface);
    m_rpc->setBsEcdhPSI(m_nodeInitializer->bsEcdhPsi());
    m_nodeInitializer->registerRpcHandler(m_rpc);

    INIT_LOG(INFO) << LOG_DESC("init the rpc success");
}

void ProNodeInitializer::start()
{
    // start the node
    if (m_nodeInitializer)
    {
        m_nodeInitializer->start();
    }
    if (m_rpc)
    {
        m_rpc->start();
    }
}

void ProNodeInitializer::stop()
{
    if (m_rpc)
    {
        m_rpc->stop();
    }
    if (m_nodeInitializer)
    {
        m_nodeInitializer->stop();
    }
}