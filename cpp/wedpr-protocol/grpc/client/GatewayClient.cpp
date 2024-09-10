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
#include "protobuf/src/RequestConverter.h"

using namespace ppc;
using namespace ppc::proto;
using namespace grpc;
using namespace ppc::gateway;
using namespace ppc::protocol;

void GatewayClient::asyncSendMessage(RouteType routeType,
    MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID, bcos::bytes&& payload,
    long timeout, ReceiveMsgFunc callback)
{
    auto request = generateRequest(traceID, routeType, routeInfo, std::move(payload), timeout);
    auto context = std::make_shared<ClientContext>();
    auto response = std::make_shared<Error>();
    m_stub->async()->asyncSendMessage(context.get(), request.get(), response.get(),
        [callback, response](Status status) { callback(toError(status, *response)); });
}

void GatewayClient::asyncGetPeers(std::function<void(bcos::Error::Ptr, std::string)> callback)
{
    auto response = std::make_shared<PeersInfo>();
    auto context = std::make_shared<ClientContext>();
    auto request = std::make_shared<Empty>();
    m_stub->async()->asyncGetPeers(
        context.get(), request.get(), response.get(), [callback, response](Status status) {
            callback(toError(status, response->error()), response->peersinfo());
        });
}

void GatewayClient::asyncGetAgencies(
    std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback)
{
    auto response = std::make_shared<AgenciesInfo>();
    auto context = std::make_shared<ClientContext>();
    auto request = std::make_shared<Empty>();
    m_stub->async()->asyncGetAgencies(
        context.get(), request.get(), response.get(), [callback, response](Status status) {
            std::set<std::string> agencies;
            for (int i = 0; i < response->agencies_size(); i++)
            {
                agencies.insert(response->agencies(i));
            }
            callback(toError(status, response->error()), agencies);
        });
}

bcos::Error::Ptr GatewayClient::registerNodeInfo(INodeInfo::Ptr const& nodeInfo)
{
    return broadCast([nodeInfo](ChannelInfo const& channel) {
        std::unique_ptr<ppc::proto::Gateway::Stub> stub(
            ppc::proto::Gateway::NewStub(channel.channel));
        auto request = toNodeInfoRequest(nodeInfo);
        ClientContext context;
        std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
        auto status = stub->registerNodeInfo(&context, *request, response.get());
        auto result = toError(status, *response);
        return result;
    });
}

bcos::Error::Ptr GatewayClient::unRegisterNodeInfo(bcos::bytesConstRef nodeID)
{
    return broadCast([nodeID](ChannelInfo const& channel) {
        std::unique_ptr<ppc::proto::Gateway::Stub> stub(
            ppc::proto::Gateway::NewStub(channel.channel));
        auto request = toNodeInfoRequest(nodeID, "");
        ClientContext context;
        std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
        auto status = stub->unRegisterNodeInfo(&context, *request, response.get());
        return toError(status, *response);
    });
}
bcos::Error::Ptr GatewayClient::registerTopic(bcos::bytesConstRef nodeID, std::string const& topic)
{
    return broadCast([nodeID, topic](ChannelInfo const& channel) {
        std::unique_ptr<ppc::proto::Gateway::Stub> stub(
            ppc::proto::Gateway::NewStub(channel.channel));
        auto request = toNodeInfoRequest(nodeID, topic);
        ClientContext context;
        std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
        auto status = stub->registerTopic(&context, *request, response.get());
        return toError(status, *response);
    });
}

bcos::Error::Ptr GatewayClient::unRegisterTopic(
    bcos::bytesConstRef nodeID, std::string const& topic)
{
    return broadCast([nodeID, topic](ChannelInfo const& channel) {
        std::unique_ptr<ppc::proto::Gateway::Stub> stub(
            ppc::proto::Gateway::NewStub(channel.channel));
        auto request = toNodeInfoRequest(nodeID, topic);
        ClientContext context;
        std::shared_ptr<ppc::proto::Error> response = std::make_shared<ppc::proto::Error>();
        auto status = stub->unRegisterTopic(&context, *request, response.get());
        return toError(status, *response);
    });
}