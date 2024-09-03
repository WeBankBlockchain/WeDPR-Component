/**
 *  Copyright (C) 2021 FISCO BCOS.
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
 * @file Common.h
 * @author: yujiechen
 * @date 2021-04-12
 */
#pragma once
#include "Service.pb.h"
#include "ppc-framework/protocol/INodeInfo.h"
#include "ppc-framework/protocol/Message.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <memory>
namespace ppc::protocol
{
inline std::shared_ptr<ppc::proto::SendedMessageRequest> generateRequest(
    ppc::protocol::RouteType routeType, ppc::protocol::MessageOptionalHeader::Ptr const& routeInfo,
    bcos::bytes&& payload, long timeout)
{
    auto request = std::make_shared<ppc::proto::SendedMessageRequest>();
    request->set_routetype(uint16_t(routeType));
    // set the route information
    request->mutable_routeinfo()->set_topic(routeInfo->topic());
    request->mutable_routeinfo()->set_componenttype(routeInfo->componentType());
    request->mutable_routeinfo()->set_srcnode(
        routeInfo->srcNode().data(), routeInfo->srcNode().size());
    request->mutable_routeinfo()->set_dstnode(
        routeInfo->dstNode().data(), routeInfo->dstNode().size());
    request->mutable_routeinfo()->set_dstinst(
        routeInfo->dstInst().data(), routeInfo->dstInst().size());
    // set the payload(TODO: optimize here)
    request->set_payload(payload.data(), payload.size());
    request->set_timeout(timeout);
    return request;
}

inline std::shared_ptr<ppc::proto::NodeInfo> toNodeInfoRequest(
    bcos::bytesConstRef const& nodeID, std::string const& topic)
{
    auto request = std::make_shared<ppc::proto::NodeInfo>();
    request->set_nodeid(nodeID.data(), nodeID.size());
    request->set_topic(topic);
    return request;
}

inline std::shared_ptr<ppc::proto::NodeInfo> toNodeInfoRequest(
    ppc::protocol::INodeInfo::Ptr const& nodeInfo)
{
    auto request = std::make_shared<ppc::proto::NodeInfo>();
    if (!nodeInfo)
    {
        return request;
    };
    request->set_nodeid(nodeInfo->nodeID().data(), nodeInfo->nodeID().size());
    request->set_endpoint(nodeInfo->endPoint());
    auto const& components = nodeInfo->components();
    for (auto const& component : components)
    {
        request->add_components(component);
    }
    return request;
}

inline bcos::Error::Ptr toError(grpc::Status const& status, ppc::proto::Error&& error)
{
    if (!status.ok())
    {
        return std::make_shared<bcos::Error>((int32_t)status.error_code(), status.error_message());
    }
    if (error.errorcode() == 0)
    {
        return nullptr;
    }
    return std::make_shared<bcos::Error>(error.errorcode(), error.errormessage());
}
};  // namespace ppc::protocol