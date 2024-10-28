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
 * @file MessageHeaderImpl.cpp
 * @author: yujiechen
 * @date 2024-08-23
 */
#include "MessageHeaderImpl.h"
#include "ppc-framework/Common.h"
#include "ppc-utilities/Utilities.h"
#include <boost/asio/detail/socket_ops.hpp>

using namespace ppc::protocol;
using namespace bcos;
using namespace ppc;

void MessageOptionalHeaderImpl::encode(bcos::bytes& buffer) const
{
    // the componentType
    uint16_t componentTypeLen =
        boost::asio::detail::socket_ops::host_to_network_short(m_componentType.size());
    buffer.insert(buffer.end(), (byte*)&componentTypeLen, (byte*)&componentTypeLen + 2);
    buffer.insert(buffer.end(), m_componentType.begin(), m_componentType.end());
    // the source nodeID that send the message
    uint16_t srcNodeLen = boost::asio::detail::socket_ops::host_to_network_short(m_srcNode.size());
    buffer.insert(buffer.end(), (byte*)&srcNodeLen, (byte*)&srcNodeLen + 2);
    buffer.insert(buffer.end(), m_srcNode.begin(), m_srcNode.end());
    // the source agency
    uint16_t srcInstLen = boost::asio::detail::socket_ops::host_to_network_short(m_srcInst.size());
    buffer.insert(buffer.end(), (byte*)&srcInstLen, (byte*)&srcInstLen + 2);
    buffer.insert(buffer.end(), m_srcInst.begin(), m_srcInst.end());
    // the target nodeID that should receive the message
    uint16_t dstNodeLen =
        boost::asio::detail::socket_ops::host_to_network_short(m_dstNodePtr.size());
    buffer.insert(buffer.end(), (byte*)&dstNodeLen, (byte*)&dstNodeLen + 2);
    buffer.insert(buffer.end(), m_dstNodePtr.begin(), m_dstNodePtr.end());
    // the target agency that need receive the message
    uint16_t dstInstLen = boost::asio::detail::socket_ops::host_to_network_short(m_dstInst.size());
    buffer.insert(buffer.end(), (byte*)&dstInstLen, (byte*)&dstInstLen + 2);
    buffer.insert(buffer.end(), m_dstInst.begin(), m_dstInst.end());
    // the topic
    uint16_t topicLen = boost::asio::detail::socket_ops::host_to_network_short(m_topic.size());
    buffer.insert(buffer.end(), (byte*)&topicLen, (byte*)&topicLen + 2);
    buffer.insert(buffer.end(), m_topic.begin(), m_topic.end());
}


int64_t MessageOptionalHeaderImpl::decode(bcos::bytesConstRef data, uint64_t const _offset)
{
    auto offset = _offset;
    CHECK_OFFSET_WITH_THROW_EXCEPTION(offset, data.size());
    // the componentType
    auto pointer = data.data() + offset;
    m_componentType = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    bcos::bytes componentType;
    offset = decodeNetworkBuffer(
        componentType, data.data(), data.size(), (pointer - data.data()), false);
    m_componentType = std::string(componentType.begin(), componentType.end());
    // srcNode
    offset = decodeNetworkBuffer(m_srcNode, data.data(), data.size(), offset, false);
    // source inst
    offset = decodeNetworkBuffer(m_srcInst, data.data(), data.size(), offset, false);
    //  dstNode
    offset = decodeNetworkBuffer(m_dstNode, data.data(), data.size(), offset, false);
    m_dstNodePtr = bcos::bytesConstRef((bcos::byte*)m_dstNode.data(), m_dstNode.size());
    // dstInst
    offset = decodeNetworkBuffer(m_dstInst, data.data(), data.size(), offset, false);
    // topic
    offset = decodeNetworkBuffer(m_topic, data.data(), data.size(), offset, false);
    return offset;
}

void MessageHeaderImpl::encode(bcos::bytes& buffer) const
{
    buffer.clear();
    // the version, 2Bytes
    uint16_t version = boost::asio::detail::socket_ops::host_to_network_short(m_version);
    buffer.insert(buffer.end(), (byte*)&version, (byte*)&version + 2);
    // the packetType, 2Bytes
    uint16_t packetType = boost::asio::detail::socket_ops::host_to_network_short(m_packetType);
    buffer.insert(buffer.end(), (byte*)&packetType, (byte*)&packetType + 2);
    // the ttl, 2Bytes
    uint16_t ttl = boost::asio::detail::socket_ops::host_to_network_short(m_ttl);
    buffer.insert(buffer.end(), (byte*)&ttl, (byte*)&ttl + 2);
    // the ext, 2Bytes
    uint16_t ext = boost::asio::detail::socket_ops::host_to_network_short(m_ext);
    buffer.insert(buffer.end(), (byte*)&ext, (byte*)&ext + 2);
    // the traceID, 2+Bytes
    uint16_t traceIDLen = boost::asio::detail::socket_ops::host_to_network_short(m_traceID.size());
    buffer.insert(buffer.end(), (byte*)&traceIDLen, (byte*)&traceIDLen + 2);
    if (m_traceID.size() > 0)
    {
        buffer.insert(buffer.end(), m_traceID.begin(), m_traceID.end());
    }
    // srcGwNode, 2+Bytes
    uint16_t srcGwNodeLen =
        boost::asio::detail::socket_ops::host_to_network_short(m_srcGwNode.size());
    buffer.insert(buffer.end(), (byte*)&srcGwNodeLen, (byte*)&srcGwNodeLen + 2);
    if (m_srcGwNode.size() > 0)
    {
        buffer.insert(buffer.end(), m_srcGwNode.begin(), m_srcGwNode.end());
    }
    // dstGwNode, 2+Bytes
    uint16_t dstGwNodeLen =
        boost::asio::detail::socket_ops::host_to_network_short(m_dstGwNode.size());
    buffer.insert(buffer.end(), (byte*)&dstGwNodeLen, (byte*)&dstGwNodeLen + 2);
    if (m_dstGwNode.size() > 0)
    {
        buffer.insert(buffer.end(), m_dstGwNode.begin(), m_dstGwNode.end());
    }
    if (!hasOptionalField())
    {
        m_length = buffer.size();
        return;
    }
    // encode the optionalField
    m_optionalField->encode(buffer);
    m_length = buffer.size();
}

int64_t MessageHeaderImpl::decode(bcos::bytesConstRef data)
{
    if (data.size() < MESSAGE_MIN_LENGTH)
    {
        BOOST_THROW_EXCEPTION(
            WeDPRException() << errinfo_comment("Malform message for too small!"));
    }
    auto pointer = data.data();
    // the version
    m_version = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the pacektType
    m_packetType = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the ttl
    m_ttl = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the ext
    m_ext = boost::asio::detail::socket_ops::network_to_host_short(*((uint16_t*)pointer));
    pointer += 2;
    // the traceID
    auto offset =
        decodeNetworkBuffer(m_traceID, data.data(), data.size(), (pointer - data.data()), false);
    // srcGwNode
    offset = decodeNetworkBuffer(m_srcGwNode, data.data(), data.size(), offset, false);
    // dstGwNode
    offset = decodeNetworkBuffer(m_dstGwNode, data.data(), data.size(), offset, false);
    // optionalField
    if (hasOptionalField())
    {
        offset = m_optionalField->decode(data, offset);
    }
    m_length = offset;
    return offset;
}

uint16_t MessageHeaderImpl::routeType() const
{
    if (m_ext & (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByNodeID)
    {
        return (uint16_t)RouteType::ROUTE_THROUGH_NODEID;
    }
    if (m_ext & (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByComponent)
    {
        return (uint16_t)RouteType::ROUTE_THROUGH_COMPONENT;
    }
    if (m_ext & (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByAgency)
    {
        return (uint16_t)RouteType::ROUTE_THROUGH_AGENCY;
    }
    if (m_ext & (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByTopic)
    {
        return (uint16_t)RouteType::ROUTE_THROUGH_TOPIC;
    }
    // default is route though nodeID
    return (uint16_t)RouteType::ROUTE_THROUGH_NODEID;
}

void MessageHeaderImpl::setRouteType(ppc::protocol::RouteType type)
{
    switch (type)
    {
    case RouteType::ROUTE_THROUGH_NODEID:
        m_ext |= (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByNodeID;
        break;
    case RouteType::ROUTE_THROUGH_COMPONENT:
        m_ext |= (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByComponent;
        break;
    case RouteType::ROUTE_THROUGH_AGENCY:
        m_ext |= (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByAgency;
        break;
    case RouteType::ROUTE_THROUGH_TOPIC:
        m_ext |= (uint16_t)ppc::gateway::GatewayMsgExtFlag::RouteByTopic;
        break;
    default:
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "Invalid route type: " + std::to_string((uint16_t)type)));
    }
}