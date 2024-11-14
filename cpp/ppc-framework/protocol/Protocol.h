/*
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
 * @file Protocol.h
 * @author: yujiechen
 * @date 2022-10-13
 */
#pragma once

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN32_)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "ppc-framework/Common.h"
#include <bcos-utilities/Log.h>
#include <map>
#include <memory>
#include <sstream>
#include <string>

namespace ppc::protocol
{
// the task type
enum class TaskType : uint8_t
{
    PSI = 0x00,
    PIR = 0x01,
};

inline std::ostream& operator<<(std::ostream& _out, TaskType const& _type)
{
    switch (_type)
    {
    case TaskType::PSI:
        _out << "PSI";
        break;
    case TaskType::PIR:
        _out << "PIR";
        break;
    default:
        _out << "UnknownTaskType";
        break;
    }
    return _out;
}

// the PSIAlgorithm type
enum class TaskAlgorithmType : uint8_t
{
    // PSI implementation for https://eprint.iacr.org/2020/729.pdf(Private Set Intersection in the
    // Internet Setting from Lightweight Oblivious PRF)
    CM_PSI_2PC = 0x00,
    // PSI implementation for https://eprint.iacr.org/2017/677.pdf(Faster Unbalanced Private Set
    // Intersection)
    RA_PSI_2PC = 0x01,
    // PSI implementation for https://eprint.iacr.org/2021/1116(Labeled PSI from Homomorphic
    // Encryption with Reduced Computation and Communication)
    LABELED_PSI_2PC = 0x02,
    ECDH_PSI_2PC = 0x03,
    ECDH_PSI_MULTI = 0x04,
    ECDH_PSI_CONN = 0x05,
    OT_PIR_2PC = 0x10,
    BS_ECDH_PSI = 0x06,
};

inline std::ostream& operator<<(std::ostream& _out, TaskAlgorithmType const& _type)
{
    switch (_type)
    {
    case TaskAlgorithmType::CM_PSI_2PC:
        _out << "CM_PSI_2PC";
        break;
    case TaskAlgorithmType::RA_PSI_2PC:
        _out << "RA_PSI_2PC";
        break;
    case TaskAlgorithmType::LABELED_PSI_2PC:
        _out << "LABELED_PSI_2PC";
        break;
    case TaskAlgorithmType::ECDH_PSI_2PC:
        _out << "ECDH_PSI_2PC";
        break;
    case TaskAlgorithmType::OT_PIR_2PC:
        _out << "OT_PIR_2PC";
        break;
    case TaskAlgorithmType::ECDH_PSI_MULTI:
        _out << "ECDH_PSI_MULTI";
        break;
    case TaskAlgorithmType::ECDH_PSI_CONN:
        _out << "ECDH_PSI_CONN";
        break;
    default:
        _out << "UnknownPSIAlgorithm";
        break;
    }
    return _out;
}

// the type of the data resource, now support FILE/HDFS, will support SQL in the future
enum class DataResourceType : uint16_t
{
    FILE,
    MySQL,
    HDFS
};

enum class CacheType : uint16_t
{
    Redis
};

enum class TaskStatus
{
    PENDING,
    RUNNING,
    PAUSING,
    FAILED,
    COMPLETED
};

inline bool isExecutable(TaskStatus _status)
{
    return _status == TaskStatus::PENDING || _status == TaskStatus::PAUSING ||
           _status == TaskStatus::RUNNING;
}

inline bool isNotExecutable(TaskStatus _status)
{
    return _status == TaskStatus::FAILED || _status == TaskStatus::COMPLETED;
}

inline std::string toString(TaskStatus _status)
{
    switch (_status)
    {
    case TaskStatus::PENDING:
        return "PENDING";
    case TaskStatus::RUNNING:
        return "RUNNING";
    case TaskStatus::PAUSING:
        return "PAUSING";
    case TaskStatus::FAILED:
        return "FAILED";
    case TaskStatus::COMPLETED:
        return "COMPLETED";
    default:
        return "UNKNOWN";
    }
}

inline TaskStatus fromString(const std::string& _str)
{
    static const std::map<std::string, TaskStatus> statusMap = {{"PENDING", TaskStatus::PENDING},
        {"RUNNING", TaskStatus::RUNNING}, {"PAUSING", TaskStatus::PAUSING},
        {"FAILED", TaskStatus::FAILED}, {"COMPLETED", TaskStatus::COMPLETED}};

    auto it = statusMap.find(_str);
    if (it != statusMap.end())
    {
        return it->second;
    }
    else
    {
        return TaskStatus::FAILED;
    }
}

inline std::ostream& operator<<(std::ostream& _out, DataResourceType const& _type)
{
    switch (_type)
    {
    case DataResourceType::FILE:
        _out << "FILE";
        break;
    case DataResourceType::MySQL:
        _out << "MySQL";
        break;
    case DataResourceType::HDFS:
        _out << "HDFS";
        break;
    default:
        _out << "UnknownDataResourceType";
        break;
    }
    return _out;
}

// the party type
// RA involves Server and Client
enum class PartyType : uint16_t
{
    Client,
    Server,
};
inline std::ostream& operator<<(std::ostream& _out, PartyType const& _type)
{
    switch (_type)
    {
    case PartyType::Client:
        _out << "Client";
        break;
    case PartyType::Server:
        _out << "Server";
        break;
    default:
        _out << "UnknownPartyType";
        break;
    }
    return _out;
}

enum class PartiesType : uint16_t
{
    Calculator,
    Partner,
    Master,
};

inline std::ostream& operator<<(std::ostream& _out, PartiesType const& _type)
{
    switch (_type)
    {
    case PartiesType::Calculator:
        _out << "Calculator";
        break;
    case PartiesType::Partner:
        _out << "Partner";
        break;
    case PartiesType::Master:
        _out << "Master";
        break;
    default:
        _out << "UnknownPartiesType";
        break;
    }
    return _out;
}

enum class NodeArch : uint8_t
{
    AIR,  // all in one
    PRO   // gateway and front are independent
};

inline std::ostream& operator<<(std::ostream& _out, NodeArch const& _type)
{
    switch (_type)
    {
    case NodeArch::AIR:
        _out << "AIR";
        break;
    case NodeArch::PRO:
        _out << "PRO";
        break;
    default:
        _out << "UnknownNodeArch";
        break;
    }
    return _out;
}

// define the ppc ret-code
enum PPCRetCode : int
{
    SUCCESS = 0,
    EXCEPTION = -100,
    TIMEOUT = -101,
    DECODE_PPC_MESSAGE_ERROR = -102,
    NETWORK_ERROR = -103,
    NOTIFY_TASK_ERROR = -104,
    REGISTER_GATEWAY_URL_ERROR = -105,

    // the storage error
    READ_RPC_STATUS_ERROR = -200,
    WRITE_RPC_STATUS_ERROR = -201
};

// the supported ecc-curve
enum class ECCCurve : int8_t
{
    ED25519,
    SM2,
    SECP256K1,  // todo: FourQ
    P256,
    IPP_X25519,
};

// the supported oprf
enum class OprfType : int8_t
{
    EcdhOprf,
};

inline std::ostream& operator<<(std::ostream& _out, ECCCurve const& _type)
{
    switch (_type)
    {
    case ECCCurve::ED25519:
        _out << "ED25519";
        break;
    case ECCCurve::SM2:
        _out << "SM2";
        break;
    case ECCCurve::SECP256K1:
        _out << "SECP256K1";
        break;
    case ECCCurve::P256:
        _out << "P256";
        break;
    case ECCCurve::IPP_X25519:
        _out << "IPP_X25519";
        break;
    default:
        _out << "UnknownEccCurve";
        break;
    }
    return _out;
}

enum class HashImplName : int8_t
{
    SHA256,
    SHA512,
    SM3,
    MD5,
    BLAKE2b  // TODO: sha3
    // TODO: blake3(https://github.com/BLAKE3-team/BLAKE3)
};

enum class PRNGImplName : int8_t
{
    AES,
    BLAKE2b,
};

enum class SymCryptoImplName : int8_t
{
    AES,
    DES_EDE3,
    SM4,
};

enum class DataPaddingType : int
{
    NONE = 0,
    PKCS7 = 1,
    ISO7816_4 = 2,
    ANSI923 = 3,
    ISO10126 = 4,
    ZERO = 5,
};


inline std::ostream& operator<<(std::ostream& _out, HashImplName const& _type)
{
    switch (_type)
    {
    case HashImplName::SHA256:
        _out << "SHA256";
        break;
    case HashImplName::SHA512:
        _out << "SHA512";
        break;
    case HashImplName::SM3:
        _out << "SM3";
        break;
    case HashImplName::MD5:
        _out << "MD5";
        break;
    case HashImplName::BLAKE2b:
        _out << "BLAKE2b";
        break;
    default:
        _out << "UnknownHashImpl";
        break;
    }
    return _out;
}

enum class MessageType : uint16_t
{
    RpcRequest = 0x1000,  // the rpc request type
};

// option used for sql-connection
struct SQLConnectionOption
{
    using Ptr = std::shared_ptr<SQLConnectionOption>;
    std::string host;
    int port;
    std::string user;
    std::string password;
    std::string database;
    void check() const
    {
        if (host.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid SQL option: Must set the host!"));
        }
        if (user.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid SQL option: Must set the user!"));
        }
        if (password.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid SQL option: Must set the password!"));
        }
        if (database.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid SQL option: Must set the database!"));
        }
        if (port == 0 || port > 65535)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid SQL Option, Must set valid port!"));
        }
    }

    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("host", host) << LOG_KV("port", port) << LOG_KV("user", user)
            << LOG_KV("database", database);
        return oss.str();
    }
};

//// the hdfs connection-option
struct FileStorageConnectionOption
{
    using Ptr = std::shared_ptr<FileStorageConnectionOption>;
    std::string nameNode;
    uint16_t nameNodePort;
    std::string userName;
    std::string token;
    // replace-datanode-on-failure
    bool replaceDataNodeOnFailure = false;
    // the default connection-timeout for the hdfs is 1000ms
    uint16_t connectionTimeout = 1000;

    void check() const
    {
        if (nameNode.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid HDFS Option, Must set the nameNode!"));
        }
        if (userName.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid HDFS Option, Must set the userName!"));
        }
        if (nameNodePort == 0 || nameNodePort > 65535)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid HDFS Option, Must set valid namenodeport!"));
        }
    }
    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("nameNode", nameNode) << LOG_KV("nameNodePort", nameNodePort)
            << LOG_KV("user", userName) << LOG_KV("token", token)
            << LOG_KV("replace-datanode-on-failure", replaceDataNodeOnFailure)
            << LOG_KV("connectionTimeout", connectionTimeout);
        return oss.str();
    }
};

struct RemoteStorageConnectionOption
{
    using Ptr = std::shared_ptr<RemoteStorageConnectionOption>;
    std::string host;
    uint16_t port;
    std::string legalPerson;
    std::string user;
    std::string password;
    std::string obInitServer;
    std::string monitorServer;
    uint32_t timeoutSec = 600;
    int saveCount = 3;  // month
    std::string logPath;

    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("host", host) << LOG_KV("port", port) << LOG_KV("legalPerson", legalPerson)
            << LOG_KV("user", user) << LOG_KV("password", password)
            << LOG_KV("obInitServer", obInitServer) << LOG_KV("monitorServer", monitorServer)
            << LOG_KV("timeoutSec", timeoutSec) << LOG_KV("saveCount", saveCount)
            << LOG_KV("logPath", logPath);
        return oss.str();
    }
};

struct GatewayInfo
{
    std::string agencyID;
    std::string endpoint;
};

struct FileInfo
{
    using Ptr = std::shared_ptr<FileInfo>;
    std::string path;
    std::string bizSeqNo;
    std::string fileID;
    std::string fileMd5;
    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("path", path) << LOG_KV("bizSeqNo", bizSeqNo) << LOG_KV("fileID", fileID)
            << LOG_KV("fileMd5", fileMd5);
        return oss.str();
    }
};
}  // namespace ppc::protocol
