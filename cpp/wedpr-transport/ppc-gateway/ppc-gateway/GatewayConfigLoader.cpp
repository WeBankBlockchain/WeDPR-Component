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
 * @file GatewayConfigLoader.cpp
 * @author: yujiechen
 * @date 2024-08-26
 */
#include "GatewayConfigLoader.h"
#include "Common.h"
#include "bcos-utilities/FileUtility.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <json/json.h>

using namespace ppc;
using namespace bcos;
using namespace ppc::tools;
using namespace ppc::gateway;
using namespace bcos::boostssl;

// load p2p connected peers
void GatewayConfigLoader::loadP2pConnectedNodes()
{
    std::string nodeFilePath =
        m_config->gatewayConfig().nodePath + "/" + m_config->gatewayConfig().nodeFileName;
    // load p2p connected nodes
    auto jsonContent = readContentsToString(boost::filesystem::path(nodeFilePath));
    if (!jsonContent || jsonContent->empty())
    {
        BOOST_THROW_EXCEPTION(
            WeDPRException() << errinfo_comment(
                "loadP2pConnectedNodes: unable to read nodes json file, path=" + nodeFilePath));
    }

    parseConnectedJson(*jsonContent.get(), *m_nodeIPEndpointSet);
    GATEWAY_LOG(INFO) << LOG_DESC("loadP2pConnectedNodes success!")
                      << LOG_KV("nodePath", m_config->gatewayConfig().nodePath)
                      << LOG_KV("nodeFileName", m_config->gatewayConfig().nodeFileName)
                      << LOG_KV("nodes", m_nodeIPEndpointSet->size());
}

void GatewayConfigLoader::parseConnectedJson(
    const std::string& _json, EndPointSet& _nodeIPEndpointSet)
{
    /*
    {"nodes":["127.0.0.1:30355","127.0.0.1:30356"}]}
    */
    Json::Value root;
    Json::Reader jsonReader;
    try
    {
        if (!jsonReader.parse(_json, root))
        {
            GATEWAY_LOG(ERROR) << "unable to parse connected nodes json" << LOG_KV("json:", _json);
            BOOST_THROW_EXCEPTION(
                WeDPRException() << errinfo_comment("GatewayConfig: unable to parse p2p "
                                                    "connected nodes json"));
        }
        Json::Value jNodes = root["nodes"];
        if (jNodes.isArray())
        {
            unsigned int jNodesSize = jNodes.size();
            for (unsigned int i = 0; i < jNodesSize; i++)
            {
                std::string host = jNodes[i].asString();

                NodeIPEndpoint endpoint;
                hostAndPort2Endpoint(host, endpoint);
                _nodeIPEndpointSet.insert(endpoint);

                GATEWAY_LOG(INFO) << LOG_DESC("add one connected node") << LOG_KV("host", host);
            }
        }
    }
    catch (const std::exception& e)
    {
        GATEWAY_LOG(ERROR) << LOG_KV(
            "parseConnectedJson error: ", boost::diagnostic_information(e));
        BOOST_THROW_EXCEPTION(e);
    }
}

bool GatewayConfigLoader::isValidPort(int port)
{
    if (port <= 1024 || port > 65535)
        return false;
    return true;
}

void GatewayConfigLoader::hostAndPort2Endpoint(const std::string& _host, NodeIPEndpoint& _endpoint)
{
    std::string ip;
    uint16_t port;

    std::vector<std::string> s;
    boost::split(s, _host, boost::is_any_of("]"), boost::token_compress_on);
    if (s.size() == 2)
    {  // ipv6
        ip = s[0].data() + 1;
        port = boost::lexical_cast<int>(s[1].data() + 1);
    }
    else if (s.size() == 1)
    {  // ipv4
        std::vector<std::string> v;
        boost::split(v, _host, boost::is_any_of(":"), boost::token_compress_on);
        if (v.size() < 2)
        {
            BOOST_THROW_EXCEPTION(
                WeDPRException() << errinfo_comment("GatewayConfig: invalid host , host=" + _host));
        }
        ip = v[0];
        port = boost::lexical_cast<int>(v[1]);
    }
    else
    {
        GATEWAY_LOG(ERROR) << LOG_DESC("not valid host value") << LOG_KV("host", _host);
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "GatewayConfig: the host is invalid, host=" + _host));
    }

    if (!isValidPort(port))
    {
        GATEWAY_LOG(ERROR) << LOG_DESC("the port is not valid") << LOG_KV("port", port);
        BOOST_THROW_EXCEPTION(
            WeDPRException() << errinfo_comment(
                "GatewayConfig: the port is invalid, port=" + std::to_string(port)));
    }

    boost::system::error_code ec;
    boost::asio::ip::address ip_address = boost::asio::ip::make_address(ip, ec);
    if (ec.value() != 0)
    {
        GATEWAY_LOG(ERROR) << LOG_DESC("the host is invalid, make_address error")
                           << LOG_KV("host", _host);
        BOOST_THROW_EXCEPTION(
            WeDPRException() << errinfo_comment(
                "GatewayConfig: the host is invalid make_address error, host=" + _host));
    }
    _endpoint = NodeIPEndpoint{ip_address, port};
}
