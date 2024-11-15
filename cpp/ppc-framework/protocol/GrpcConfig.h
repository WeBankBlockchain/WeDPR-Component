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
        if (maxSendMessageSize > c_maxMsgSize)
        {
            BOOST_THROW_EXCEPTION(
                WeDPRException() << bcos::errinfo_comment(
                    "The MaxSendMessageSize limit is " + std::to_string(c_maxMsgSize)));
        }
        m_maxSendMessageSize = maxSendMessageSize;
    }
    void setMaxReceivedMessageSize(uint64_t maxReceivedMessageSize)
    {
        if (maxReceivedMessageSize > c_maxMsgSize)
        {
            BOOST_THROW_EXCEPTION(
                WeDPRException() << bcos::errinfo_comment(
                    "The MaxReceivedMessageSize limit is " + std::to_string(c_maxMsgSize)));
        }
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

    uint64_t maxMsgSize() const { return m_maxMsgSize; }
    void setMaxMsgSize(uint64_t maxMsgSize)
    {
        if (maxMsgSize > c_maxMsgSize)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "The maxMsgSize limit is " + std::to_string(c_maxMsgSize)));
        }
        m_maxMsgSize = maxMsgSize;
    }

protected:
    bool m_enableHealthCheck = true;
    std::string m_loadBalancePolicy = "round_robin";
    bool m_enableDnslookup = false;

    // Note: grpc use int to set the maxMsgSize
    uint64_t const c_maxMsgSize = INT_MAX;

    // the max send message size in bytes
    uint64_t m_maxSendMessageSize = c_maxMsgSize;
    // the max received message size in bytes
    uint64_t m_maxReceivedMessageSize = c_maxMsgSize;
    // the max msg size
    uint64_t m_maxMsgSize = c_maxMsgSize;
    int m_compressAlgorithm = 0;
};

class GrpcServerConfig
{
public:
    using Ptr = std::shared_ptr<GrpcServerConfig>;
    GrpcServerConfig() { m_grpcConfig = std::make_shared<GrpcConfig>(); }
    GrpcServerConfig(EndPoint endPoint, bool enableHealthCheck) : GrpcServerConfig()
    {
        m_endPoint = std::move(endPoint);
        m_enableHealthCheck = enableHealthCheck;
    }
    virtual ~GrpcServerConfig() = default;

    std::string listenEndPoint() const { return m_endPoint.listenEndPoint(); }

    void setEndPoint(EndPoint endPoint) { m_endPoint = endPoint; }
    void setEnableHealthCheck(bool enableHealthCheck) { m_enableHealthCheck = enableHealthCheck; }

    EndPoint const& endPoint() const { return m_endPoint; }
    EndPoint& mutableEndPoint() { return m_endPoint; }
    bool enableHealthCheck() const { return m_enableHealthCheck; }

    GrpcConfig::Ptr const& grpcConfig() const { return m_grpcConfig; }

protected:
    ppc::protocol::EndPoint m_endPoint;
    bool m_enableHealthCheck = true;
    // the grpc config
    GrpcConfig::Ptr m_grpcConfig;
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