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
 * @file HttpClient.cpp
 * @author: shawnhe
 * @date 2023-07-25
 */

#include "HttpClient.h"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;
using namespace boost::system;

bcos::bytes ppc::http::HttpClient::post(const std::string& path,
    const std::map<std::string, std::string>& headers, const bcos::bytes& body)
{
    return query(boost::beast::http::verb::post, path, headers, body);
}

bcos::bytes ppc::http::HttpClient::get(const std::string& path,
    const std::map<std::string, std::string>& headers, const bcos::bytes& body)
{
    return query(boost::beast::http::verb::get, path, headers, body);
}

bcos::bytes ppc::http::HttpClient::query(boost::beast::http::verb method, const std::string& path,
    const std::map<std::string, std::string>& headers, const bcos::bytes& body)
{
    boost::system::error_code ec;
    // Declare a container to hold the response
    boost::beast::http::response<boost::beast::http::dynamic_body> response;

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Make the connection on the IP address we get from a lookup
    if (m_enableSsl)
    {
        boost::asio::ssl::stream<tcp::socket> socket_stream(m_ioContext, m_sslContext);
        boost::asio::connect(socket_stream.next_layer(), m_endpoints);
        socket_stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        boost::beast::http::request<boost::beast::http::string_body> request{
            boost::beast::http::verb::post, path, m_version};
        request.set(boost::beast::http::field::host, m_serverHost);
        request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        request.set(boost::beast::http::field::content_length, std::to_string(body.size()));

        for (const auto& header : headers)
        {
            request.set(header.first, header.second);
        }

        request.body() = std::string(reinterpret_cast<const char*>(body.data()), body.size());

        // Send the HTTP request to the remote host
        boost::beast::http::write(socket_stream, request);


        // Receive the HTTP response
        boost::beast::http::read(socket_stream, buffer, response);

        // Gracefully close the socket
        socket_stream.shutdown(ec);
    }
    else
    {
        boost::asio::ip::tcp::socket newSocket(m_ioContext);
        boost::asio::connect(newSocket, m_endpoints.begin(), m_endpoints.end());

        // Set up an HTTP GET request message
        boost::beast::http::request<boost::beast::http::string_body> request{
            boost::beast::http::verb::post, path, m_version};
        request.set(boost::beast::http::field::host, m_serverHost);
        request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        request.set(boost::beast::http::field::content_length, std::to_string(body.size()));

        for (const auto& header : headers)
        {
            request.set(header.first, header.second);
        }

        request.body() = std::string(reinterpret_cast<const char*>(body.data()), body.size());

        // Send the HTTP request to the remote host
        boost::beast::http::write(newSocket, request);

        // Receive the HTTP response
        boost::beast::http::read(newSocket, buffer, response);

        // Gracefully close the socket
        newSocket.shutdown(tcp::socket::shutdown_both, ec);
    }

    // not_connected happens sometimes
    // so don't bother reporting it.
    if (ec && ec != boost::system::errc::not_connected)
        throw boost::system::system_error{ec};

    // Copy the response body to a vector<char>
    std::vector<uint8_t> response_data;
    response_data.reserve(boost::asio::buffer_size(response.body().data()));
    response_data.assign(boost::asio::buffers_begin(response.body().data()),
        boost::asio::buffers_end(response.body().data()));

    return response_data;
}
