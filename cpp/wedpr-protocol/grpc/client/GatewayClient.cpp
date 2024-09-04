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
 * @file GatewayClient.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#include "GatewayClient.h"
#include "Common.h"
#include "Service.grpc.pb.h"
#include "protobuf/RequestConverter.h"

using namespace ppc;
using namespace ppc::proto;
using namespace grpc;
using namespace ppc::gateway;
using namespace ppc::protocol;

void GatewayClient::asyncSendMessage(RouteType routeType,
    MessageOptionalHeader::Ptr const& routeInfo, bcos::bytes&& payload, long timeout,
    ReceiveMsgFunc callback)
{
    auto request = generateRequest(routeType, routeInfo, std::move(payload), timeout);
    ClientContext context;
    auto response = std::make_shared<Error>();
    m_stub->async()->asyncSendMessage(&context, request.get(), response.get(),
        [callback, response](Status status) { callback(toError(status, std::move(*response))); });
}


bcos::Error::Ptr GatewayClient::registerNodeInfo(INodeInfo::Ptr const& nodeInfo)
{
    auto request = toNodeInfoRequest(nodeInfo);
    ClientContext context;
    std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
    auto status = m_stub->registerNodeInfo(&context, *request, response.get());
    return toError(status, std::move(*response));
}

bcos::Error::Ptr GatewayClient::unRegisterNodeInfo(bcos::bytesConstRef nodeID)
{
    auto request = toNodeInfoRequest(nodeID, "");
    ClientContext context;
    std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
    auto status = m_stub->unRegisterNodeInfo(&context, *request, response.get());
    return toError(status, std::move(*response));
}
bcos::Error::Ptr GatewayClient::registerTopic(bcos::bytesConstRef nodeID, std::string const& topic)
{
    auto request = toNodeInfoRequest(nodeID, topic);
    ClientContext context;
    std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
    auto status = m_stub->registerTopic(&context, *request, response.get());
    return toError(status, std::move(*response));
}

bcos::Error::Ptr GatewayClient::unRegisterTopic(
    bcos::bytesConstRef nodeID, std::string const& topic)
{
    auto request = toNodeInfoRequest(nodeID, topic);
    ClientContext context;
    std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
    auto status = m_stub->unRegisterTopic(&context, *request, response.get());
    return toError(status, std::move(*response));
}