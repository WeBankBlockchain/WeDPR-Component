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
 * @file GrpcConfig.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/EndPoint.h"
#include <memory>
#include <sstream>
#include <string>

namespace ppc::protocol
{
class GrpcServerConfig
{
public:
    using Ptr = std::shared_ptr<GrpcServerConfig>;
    GrpcServerConfig() = default;
    GrpcServerConfig(EndPoint endPoint, bool enableHealthCheck)
      : m_endPoint(std::move(endPoint)), m_enableHealthCheck(enableHealthCheck)
    {}
    std::string listenEndPoint() const { return m_endPoint.listenEndPoint(); }

    void setEndPoint(EndPoint endPoint) { m_endPoint = endPoint; }
    void setEnableHealthCheck(bool enableHealthCheck) { m_enableHealthCheck = enableHealthCheck; }

    EndPoint const& endPoint() const { return m_endPoint; }
    EndPoint& mutableEndPoint() { return m_endPoint; }
    bool enableHealthCheck() const { return m_enableHealthCheck; }

protected:
    ppc::protocol::EndPoint m_endPoint;
    bool m_enableHealthCheck = true;
};
class GrpcConfig
{
public:
    using Ptr = std::shared_ptr<GrpcConfig>;
    GrpcConfig() = default;
    virtual ~GrpcConfig() = default;

    std::string const& loadBalancePolicy() const { return m_loadBalancePolicy; }
    void setLoadBalancePolicy(std::string const& loadBalancePolicy)
    {
        m_loadBalancePolicy = loadBalancePolicy;
    }

    bool enableHealthCheck() const { return m_enableHealthCheck; }
    void setEnableHealthCheck(bool enableHealthCheck) { m_enableHealthCheck = enableHealthCheck; }
    void setEnableDnslookup(bool enableDnslookup) { m_enableDnslookup = enableDnslookup; }

    bool enableDnslookup() const { return m_enableDnslookup; }

    uint64_t maxSendMessageSize() const { return m_maxSendMessageSize; }
    uint64_t maxReceivedMessageSize() const { return m_maxReceivedMessageSize; }

    void setMaxSendMessageSize(uint64_t maxSendMessageSize)
    {
        m_maxSendMessageSize = maxSendMessageSize;
    }
    void setMaxReceivedMessageSize(uint64_t maxReceivedMessageSize)
    {
        m_maxReceivedMessageSize = maxReceivedMessageSize;
    }

    /*
    typedef enum {
    GRPC_COMPRESS_NONE = 0,
    GRPC_COMPRESS_DEFLATE,
    GRPC_COMPRESS_GZIP,
    GRPC_COMPRESS_ALGORITHMS_COUNT
    } grpc_compression_algorithm;
    */
    int compressAlgorithm() const { return m_compressAlgorithm; }

    void setCompressAlgorithm(int compressAlgorithm)
    {
        if (compressAlgorithm < 0 || compressAlgorithm > 2)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid compress algorithm, must between 0-3"));
        }
        m_compressAlgorithm = compressAlgorithm;
    }

protected:
    bool m_enableHealthCheck = true;
    std::string m_loadBalancePolicy = "round_robin";
    bool m_enableDnslookup = false;

    // the max send message size in bytes
    uint64_t m_maxSendMessageSize = 1024 * 1024 * 1024;
    // the max received message size in bytes
    uint64_t m_maxReceivedMessageSize = 1024 * 1024 * 1024;
    int m_compressAlgorithm = 0;
};

inline std::string printGrpcConfig(ppc::protocol::GrpcConfig::Ptr const& grpcConfig)
{
    if (!grpcConfig)
    {
        return "nullptr";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("loadBalancePolicy", grpcConfig->loadBalancePolicy())
                 << LOG_KV("enableHealthCheck", grpcConfig->enableHealthCheck())
                 << LOG_KV("enableDnslookup", grpcConfig->enableDnslookup())
                 << LOG_KV("maxSendMessageSize", grpcConfig->maxSendMessageSize())
                 << LOG_KV("maxReceivedMessageSize", grpcConfig->maxReceivedMessageSize())
                 << LOG_KV("compressAlgorithm", grpcConfig->compressAlgorithm());
    return stringstream.str();
}
}  // namespace ppc::protocol