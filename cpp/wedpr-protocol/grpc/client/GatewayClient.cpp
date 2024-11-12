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

GatewayClient::GatewayClient(ppc::protocol::GrpcConfig::Ptr const& grpcConfig,
    std::string const& endPoints, INodeInfoFactory::Ptr nodeInfoFactory)
  : GrpcClient(grpcConfig, endPoints),
    m_stub(ppc::proto::Gateway::NewStub(m_channel)),
    m_nodeInfoFactory(std::move(nodeInfoFactory))
{
    for (auto const& channel : m_broadcastChannels)
    {
        m_broadcastStubs.insert(
            std::make_pair(channel.endPoint, ppc::proto::Gateway::NewStub(channel.channel)));
    }
}

void GatewayClient::asyncSendMessage(RouteType routeType,
    MessageOptionalHeader::Ptr const& routeInfo, std::string const& traceID, bcos::bytes&& payload,
    long timeout, ReceiveMsgFunc callback)
{
    std::unique_ptr<ppc::proto::SendedMessageRequest> request(
        generateRequest(traceID, routeType, routeInfo, std::move(payload), timeout));
    auto context = std::make_shared<ClientContext>();
    auto response = std::make_shared<Error>();
    // lambda keeps the lifecycle for clientContext
    m_stub->async()->asyncSendMessage(context.get(), request.get(), response.get(),
        [context, traceID, callback, response](
            Status status) { callback(toError(status, *response)); });
}

std::vector<std::string> GatewayClient::selectNodesByRoutePolicy(
    RouteType routeType, MessageOptionalHeader::Ptr const& routeInfo)
{
    std::unique_ptr<ppc::proto::SelectRouteRequest> request(
        generateSelectRouteRequest(routeType, routeInfo));
    auto context = std::make_shared<ClientContext>();
    auto response = std::make_shared<NodeList>();
    // lambda keeps the lifecycle for clientContext
    auto status = m_stub->selectNodesByRoutePolicy(context.get(), *request, response.get());
    if (!status.ok())
    {
        throw std::runtime_error(
            "selectNodesByRoutePolicy failed, code: " + std::to_string(status.error_code()) +
            ", msg: " + status.error_message());
    }
    if (response->error().errorcode() != 0)
    {
        throw std::runtime_error("selectNodesByRoutePolicy failed, code: " +
                                 std::to_string(response->error().errorcode()) +
                                 ", msg: " + response->error().errormessage());
    }
    return std::vector<std::string>(response->nodelist().begin(), response->nodelist().end());
}

std::vector<ppc::protocol::INodeInfo::Ptr> GatewayClient::getAliveNodeList() const
{
    auto request = std::make_shared<Empty>();
    auto response = std::make_shared<NodeInfoList>();
    auto context = std::make_shared<ClientContext>();
    // lambda keeps the lifecycle for clientContext
    auto status = m_stub->getAliveNodeList(context.get(), *request, response.get());
    if (!status.ok())
    {
        throw std::runtime_error(
            "getAliveNodeList failed, code: " + std::to_string(status.error_code()) +
            ", msg: " + status.error_message());
    }
    if (response->error().errorcode() != 0)
    {
        throw std::runtime_error(
            "getAliveNodeList failed, code: " + std::to_string(response->error().errorcode()) +
            ", msg: " + response->error().errormessage());
    }
    return toNodeInfoList(m_nodeInfoFactory, *response);
}

void GatewayClient::asyncGetPeers(std::function<void(bcos::Error::Ptr, std::string)> callback)
{
    auto response = std::make_shared<PeersInfo>();
    auto context = std::make_shared<ClientContext>();
    auto request = std::make_shared<Empty>();
    // lambda keeps the lifecycle for clientContext
    m_stub->async()->asyncGetPeers(
        context.get(), request.get(), response.get(), [context, callback, response](Status status) {
            callback(toError(status, response->error()), response->peersinfo());
        });
}

void GatewayClient::asyncGetAgencies(std::vector<std::string> const& components,
    std::function<void(bcos::Error::Ptr, std::set<std::string>)> callback)
{
    auto response = std::make_shared<AgenciesInfo>();
    auto context = std::make_shared<ClientContext>();
    auto request = std::make_shared<Condition>();
    for (auto const& it : components)
    {
        request->add_components(it);
    }
    // lambda keeps the lifecycle for clientContext
    m_stub->async()->asyncGetAgencies(
        context.get(), request.get(), response.get(), [context, callback, response](Status status) {
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
    std::unique_ptr<ppc::proto::NodeInfo> request(new ppc::proto::NodeInfo());
    toNodeInfoRequest(request.get(), nodeInfo);
    return broadCast([&](ChannelInfo const& channel) {
        if (!m_broadcastStubs.count(channel.endPoint))
        {
            return make_shared<bcos::Error>(
                -1, "registerNodeInfo failed for not find stub for endPoint: " + channel.endPoint);
        }
        auto const& stub = m_broadcastStubs.at(channel.endPoint);

        auto context = std::make_shared<ClientContext>();
        auto response = std::make_shared<ppc::proto::Error>();
        auto status = stub->registerNodeInfo(context.get(), *request, response.get());
        auto result = toError(status, *response);
        return result;
    });
}

bcos::Error::Ptr GatewayClient::unRegisterNodeInfo(bcos::bytesConstRef nodeID)
{
    std::unique_ptr<ppc::proto::NodeInfo> request(toNodeInfoRequest(nodeID, ""));
    return broadCast([&](ChannelInfo const& channel) {
        if (!m_broadcastStubs.count(channel.endPoint))
        {
            return make_shared<bcos::Error>(-1,
                "unRegisterNodeInfo failed for not find stub for endPoint: " + channel.endPoint);
        }
        auto const& stub = m_broadcastStubs.at(channel.endPoint);

        auto context = std::make_shared<ClientContext>();
        auto response = std::make_shared<ppc::proto::Error>();
        auto status = stub->unRegisterNodeInfo(context.get(), *request, response.get());
        return toError(status, *response);
    });
}
bcos::Error::Ptr GatewayClient::registerTopic(bcos::bytesConstRef nodeID, std::string const& topic)
{
    std::unique_ptr<ppc::proto::NodeInfo> request(toNodeInfoRequest(nodeID, topic));
    return broadCast([&](ChannelInfo const& channel) {
        if (!m_broadcastStubs.count(channel.endPoint))
        {
            return make_shared<bcos::Error>(
                -1, "registerTopic failed for not find stub for endPoint: " + channel.endPoint);
        }
        auto const& stub = m_broadcastStubs.at(channel.endPoint);

        auto context = std::make_shared<ClientContext>();
        auto response = std::make_shared<ppc::proto::Error>();
        auto status = stub->registerTopic(context.get(), *request, response.get());
        return toError(status, *response);
    });
}

bcos::Error::Ptr GatewayClient::unRegisterTopic(
    bcos::bytesConstRef nodeID, std::string const& topic)
{
    std::unique_ptr<ppc::proto::NodeInfo> request(toNodeInfoRequest(nodeID, topic));
    return broadCast([&](ChannelInfo const& channel) {
        if (!m_broadcastStubs.count(channel.endPoint))
        {
            return make_shared<bcos::Error>(
                -1, "unRegisterTopic failed for not find stub for endPoint: " + channel.endPoint);
        }
        auto const& stub = m_broadcastStubs.at(channel.endPoint);
        auto context = std::make_shared<ClientContext>();
        auto response = std::make_shared<ppc::proto::Error>();
        auto status = stub->unRegisterTopic(context.get(), *request, response.get());
        return toError(status, *response);
    });
}