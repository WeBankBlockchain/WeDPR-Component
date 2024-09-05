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
 * @file EndPoint.h
 * @author: yujiechen
 * @date 2024-08-22
 */

#pragma once
#include <memory>
#include <string>
#include <vector>

namespace ppc::protocol
{
/**
 * @brief the endpoint
 *
 */
class EndPoint
{
public:
    EndPoint() = default;
    EndPoint(std::string const& host, uint16_t port) : m_host(std::move(host)), m_port(port) {}
    virtual ~EndPoint() = default;

    virtual std::string const& host() const { return m_host; }
    uint16_t port() const { return m_port; }

    void setHost(std::string host) { m_host = std::move(host); }
    void setPort(uint16_t port) { m_port = port; }
    void setListenIp(std::string const& listenIp) { m_listenIp = listenIp; }

    std::string entryPoint() const { return m_host + ":" + std::to_string(m_port); }

    std::string listenEndPoint() const { return m_listenIp + ":" + std::to_string(m_port); }

    std::string const& listenIp() const { return m_listenIp; }

private:
    // the listenIp
    std::string m_listenIp = "0.0.0.0";
    // the host
    std::string m_host;
    // the port
    uint16_t m_port;
};
}  // namespace ppc::protocol