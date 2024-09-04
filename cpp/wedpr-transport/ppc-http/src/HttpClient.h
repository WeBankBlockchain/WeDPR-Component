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
 * @file HttpClient.h
 * @author: shawnhe
 * @date 2023-07-25
 */

#pragma once
#include "bcos-utilities/Common.h"
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ppc::http
{
class HttpClient
{
public:
    using Ptr = std::shared_ptr<HttpClient>;
    HttpClient(boost::asio::io_context& ioContext, const std::string& hostOrIp, unsigned short port,
        bool enableSsl = false, unsigned short version = 11)
      : m_ioContext(ioContext),
        m_resolver(ioContext),
        m_serverHost(hostOrIp),
        m_port(port),
        m_enableSsl(enableSsl),
        m_version(version)
    {
        if (enableSsl)
        {
            m_sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
        }
        m_endpoints = m_resolver.resolve(m_serverHost, std::to_string(m_port));
    }

    bcos::bytes post(const std::string& path, const std::map<std::string, std::string>& headers,
        const bcos::bytes& body);

    bcos::bytes get(const std::string& path, const std::map<std::string, std::string>& headers,
        const bcos::bytes& body);

private:
    bcos::bytes query(boost::beast::http::verb method, const std::string& path,
        const std::map<std::string, std::string>& headers, const bcos::bytes& body);

private:
    boost::asio::io_context& m_ioContext;
    boost::asio::ip::tcp::resolver m_resolver;
    boost::asio::ssl::context m_sslContext =
        boost::asio::ssl::context(boost::asio::ssl::context::sslv23_client);

    std::string m_serverHost;
    unsigned short m_port;
    bool m_enableSsl;
    unsigned short m_version;

    boost::asio::ip::tcp::resolver::results_type m_endpoints;
};
}  // namespace ppc::http
