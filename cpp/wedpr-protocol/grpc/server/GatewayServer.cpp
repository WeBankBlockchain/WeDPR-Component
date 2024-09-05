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
 * @file GatewayServer.cpp
 * @author: yujiechen
 * @date 2024-09-03
 */
#include "GatewayServer.h"
#include "Common.h"
#include "protobuf/src/RequestConverter.h"
using namespace ppc::protocol;
using namespace grpc;

ServerUnaryReactor* GatewayServer::asyncSendMessage(CallbackServerContext* context,
    const ppc::proto::SendedMessageRequest* sendedMsg, ppc::proto::Error* reply)
{
    std::shared_ptr<ServerUnaryReactor> reactor(context->DefaultReactor());
    try
    {
        // TODO: optimize here
        bcos::bytes payloadData(sendedMsg->payload().begin(), sendedMsg->payload().end());
        auto routeInfo = generateRouteInfo(m_routeInfoBuilder, sendedMsg->routeinfo());
        m_gateway->asyncSendMessage((ppc::protocol::RouteType)sendedMsg->routetype(), routeInfo,
            sendedMsg->traceid(), std::move(payloadData), sendedMsg->timeout(),
            [reactor, reply](bcos::Error::Ptr error) {
                toSerializedError(reply, error);
                reactor->Finish(Status::OK);
            });
    }
    catch (std::exception const& e)
    {
        GATEWAY_SERVER_LOG(WARNING) << LOG_DESC("asyncSendMessage exception")
                                    << LOG_KV("error", boost::diagnostic_information(e));
        toSerializedError(reply,
            std::make_shared<bcos::Error>(-1,
                "handle message failed for : " + std::string(boost::diagnostic_information(e))));
        reactor->Finish(Status::OK);
    }
    return reactor.get();
}

ServerUnaryReactor* GatewayServer::registerNodeInfo(CallbackServerContext* context,
    const ppc::proto::NodeInfo* serializedNodeInfo, ppc::proto::Error* reply)
{
    std::shared_ptr<ServerUnaryReactor> reactor(context->DefaultReactor());
    try
    {
        auto nodeInfo = toNodeInfo(m_nodeInfoFactory, *serializedNodeInfo);
        auto result = m_gateway->registerNodeInfo(nodeInfo);
        toSerializedError(reply, result);
        reactor->Finish(Status::OK);
    }
    catch (std::exception const& e)
    {
        GATEWAY_SERVER_LOG(WARNING) << LOG_DESC("registerNodeInfo exception")
                                    << LOG_KV("error", boost::diagnostic_information(e));
        toSerializedError(reply,
            std::make_shared<bcos::Error>(-1,
                "registerNodeInfo failed for : " + std::string(boost::diagnostic_information(e))));
        reactor->Finish(Status::OK);
    }
    return reactor.get();
}

ServerUnaryReactor* GatewayServer::unRegisterNodeInfo(
    CallbackServerContext* context, const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply)
{
    std::shared_ptr<ServerUnaryReactor> reactor(context->DefaultReactor());
    try
    {
        auto result = m_gateway->unRegisterNodeInfo(
            bcos::bytesConstRef((bcos::byte*)nodeInfo->nodeid().data(), nodeInfo->nodeid().size()));
        toSerializedError(reply, result);
        reactor->Finish(Status::OK);
    }
    catch (std::exception const& e)
    {
        GATEWAY_SERVER_LOG(WARNING) << LOG_DESC("unRegisterNodeInfo exception")
                                    << LOG_KV("error", boost::diagnostic_information(e));
        toSerializedError(reply,
            std::make_shared<bcos::Error>(-1, "unRegisterNodeInfo failed for : " +
                                                  std::string(boost::diagnostic_information(e))));
        reactor->Finish(Status::OK);
    }
    return reactor.get();
}

ServerUnaryReactor* GatewayServer::registerTopic(
    CallbackServerContext* context, const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply)
{
    std::shared_ptr<ServerUnaryReactor> reactor(context->DefaultReactor());
    try
    {
        auto result = m_gateway->registerTopic(
            bcos::bytesConstRef((bcos::byte*)nodeInfo->nodeid().data(), nodeInfo->nodeid().size()),
            nodeInfo->topic());
        toSerializedError(reply, result);
        reactor->Finish(Status::OK);
    }
    catch (std::exception const& e)
    {
        GATEWAY_SERVER_LOG(WARNING) << LOG_DESC("unRegisterNodeInfo exception")
                                    << LOG_KV("error", boost::diagnostic_information(e));
        toSerializedError(reply,
            std::make_shared<bcos::Error>(
                -1, "registerTopic failed for : " + std::string(boost::diagnostic_information(e))));
        reactor->Finish(Status::OK);
    }
    return reactor.get();
}

ServerUnaryReactor* GatewayServer::unRegisterTopic(
    CallbackServerContext* context, const ppc::proto::NodeInfo* nodeInfo, ppc::proto::Error* reply)
{
    std::shared_ptr<ServerUnaryReactor> reactor(context->DefaultReactor());
    try
    {
        auto result = m_gateway->unRegisterTopic(
            bcos::bytesConstRef((bcos::byte*)nodeInfo->nodeid().data(), nodeInfo->nodeid().size()),
            nodeInfo->topic());
        toSerializedError(reply, result);
        reactor->Finish(Status::OK);
    }
    catch (std::exception const& e)
    {
        GATEWAY_SERVER_LOG(WARNING) << LOG_DESC("unRegisterTopic exception")
                                    << LOG_KV("error", boost::diagnostic_information(e));
        toSerializedError(reply,
            std::make_shared<bcos::Error>(-1,
                "unRegisterTopic failed for : " + std::string(boost::diagnostic_information(e))));
        reactor->Finish(Status::OK);
    }
    return reactor.get();
}