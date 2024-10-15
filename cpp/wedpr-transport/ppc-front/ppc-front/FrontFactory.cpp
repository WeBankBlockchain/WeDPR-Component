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
 * @file FrontFactory.cpp
 * @author: yujiechen
 * @date 2024-9-04
 */
#include "FrontFactory.h"
#include "FrontImpl.h"

using namespace ppc::front;
using namespace ppc::protocol;

IFront::Ptr FrontFactory::build(INodeInfoFactory::Ptr nodeInfoFactory,
    MessagePayloadBuilder::Ptr messageFactory, MessageOptionalHeaderBuilder::Ptr routerInfoBuilder,
    ppc::gateway::IGateway::Ptr const& gateway, FrontConfig::Ptr config)
{
    auto threadPool = std::make_shared<bcos::ThreadPool>("front", config->threadPoolSize());
    auto nodeInfo = nodeInfoFactory->build(
        bcos::bytesConstRef((bcos::byte*)config->nodeID().data(), config->nodeID().size()),
        config->selfEndPoint().entryPoint());
    FRONT_LOG(INFO) << LOG_DESC("build front") << LOG_KV("nodeID", config->nodeID())
                    << LOG_KV("endPoint", config->selfEndPoint().entryPoint());
    return std::make_shared<FrontImpl>(threadPool, nodeInfo, messageFactory, routerInfoBuilder,
        gateway, std::make_shared<boost::asio::io_service>());
}