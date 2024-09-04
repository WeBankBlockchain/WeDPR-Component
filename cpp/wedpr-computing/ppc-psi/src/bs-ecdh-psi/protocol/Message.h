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
 * @file Message.h
 * @author: shawnhe
 * @date 2023-09-21
 */

#pragma once

#include "ppc-framework/protocol/Protocol.h"
#include "ppc-framework/rpc/RpcTypeDef.h"
#include "protocol/src/JsonTaskImpl.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <json/json.h>
#include <memory>
#include <string>

namespace ppc::psi
{
struct GetTaskStatusRequest
{
    using Ptr = std::shared_ptr<GetTaskStatusRequest>;
    std::string taskID;

    GetTaskStatusRequest() = default;
    ~GetTaskStatusRequest() = default;

    explicit GetTaskStatusRequest(const Json::Value& _params) { deserialize(_params); }

    void deserialize(const Json::Value& _params)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();
    }
};

struct GetTaskStatusResponse
{
    using Ptr = std::shared_ptr<GetTaskStatusResponse>;
    std::string taskID;
    std::string status;
    uint32_t step = 0;
    uint32_t index = 0;
    uint progress = 0;
    uint32_t intersections;
    uint32_t party0Size;
    uint32_t party1Size;
    std::string timeCost;
    std::string resultFileMd5;
    std::string resultFileID;
    std::string partnerIndexFileMd5;
    std::string partnerIndexFileID;
    std::string evidenceFileMd5;
    std::string evidenceFileID;

    [[nodiscard]] Json::Value serialize() const
    {
        Json::Value root;
        root["taskID"] = taskID;
        root["status"] = status;
        root["step"] = step;
        root["index"] = index;
        root["progress"] = progress;
        if (status == protocol::toString(protocol::TaskStatus::COMPLETED))
        {
            root["timeCost"] = timeCost;
            root["intersections"] = intersections;
            root["party0Size"] = party0Size;
            root["party1Size"] = party1Size;
            if (!resultFileID.empty())
            {
                root["resultFileID"] = resultFileID;
                root["resultFileMd5"] = resultFileMd5;
            }
            if (!partnerIndexFileID.empty())
            {
                root["partnerIndexFileID"] = partnerIndexFileID;
                root["partnerIndexFileMd5"] = partnerIndexFileMd5;
            }
            if (!evidenceFileID.empty())
            {
                root["evidenceFileID"] = evidenceFileID;
                root["evidenceFileMd5"] = evidenceFileMd5;
            }
        }
        return root;
    }
};

struct KillTaskRequest
{
    using Ptr = std::shared_ptr<KillTaskRequest>;
    std::string taskID;

    KillTaskRequest() = default;
    ~KillTaskRequest() = default;

    explicit KillTaskRequest(const Json::Value& _params) { deserialize(_params); }

    void deserialize(const Json::Value& _params)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();
    }
};

struct UpdateTaskStatusRequest
{
    using Ptr = std::shared_ptr<UpdateTaskStatusRequest>;
    std::string taskID;
    std::string status;

    UpdateTaskStatusRequest() = default;
    ~UpdateTaskStatusRequest() = default;

    explicit UpdateTaskStatusRequest(const Json::Value& _params) { deserialize(_params); }

    void deserialize(const Json::Value& _params)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();

        if (!_params.isMember("status"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the status field"));
        }
        status = _params["status"].asString();
    }
};

struct RunTaskRequest
{
    using Ptr = std::shared_ptr<RunTaskRequest>;
    std::string taskID;
    bool enableAudit{true};
    uint32_t partnerInputsSize{0};
    protocol::DataResource::Ptr dataResource;

    RunTaskRequest() = default;
    ~RunTaskRequest() = default;

    explicit RunTaskRequest(const Json::Value& _params, const std::string& _prePath)
    {
        deserialize(_params, _prePath);
    }

    void deserialize(const Json::Value& _params, const std::string& _prePath)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();

        if (_params.isMember("enableAudit"))
        {
            enableAudit = _params["enableAudit"].asBool();
        }
        else
        {
            enableAudit = true;
        }
        if (_params.isMember("partnerInputsSize"))
        {
            partnerInputsSize = _params["partnerInputsSize"].asUInt();
        }
        if (!_params.isMember("data"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the data field"));
        }
        auto jsonTask = std::make_shared<protocol::JsonTaskImpl>("", _prePath);
        dataResource = jsonTask->decodeDataResource(_params["data"]);
    }

    [[nodiscard]] std::string toString() const
    {
        std::stringstream ss;
        ss << "taskID: " << taskID << ", enableAudit: " << enableAudit << ", "
           << protocol::printDataResourceInfo(dataResource) << "\n";
        return ss.str();
    }
};

struct FetchCipherRequest
{
    using Ptr = std::shared_ptr<FetchCipherRequest>;
    std::string taskID;
    uint32_t offset;
    uint32_t size;

    FetchCipherRequest() = default;
    ~FetchCipherRequest() = default;

    explicit FetchCipherRequest(const Json::Value& _params) { deserialize(_params); }

    void deserialize(const Json::Value& _params)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();

        if (!_params.isMember("offset"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the offset field"));
        }
        offset = _params["offset"].asUInt();

        if (!_params.isMember("size"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must define the size field"));
        }
        size = _params["size"].asUInt();
    }
};

struct FetchCipherResponse
{
    using Ptr = std::shared_ptr<FetchCipherResponse>;
    std::string taskID;
    uint32_t offset;
    uint32_t size;
    uint32_t total;
    std::vector<std::string> ciphers;

    [[nodiscard]] Json::Value serialize() const
    {
        Json::Value root;
        root["taskID"] = taskID;
        root["offset"] = offset;
        root["size"] = size;
        root["total"] = total;

        Json::Value cipherArray(Json::arrayValue);
        for (const std::string& cipher : ciphers)
        {
            cipherArray.append(cipher);
        }
        root["ciphers"] = cipherArray;

        return root;
    }
};

struct SendEcdhCipherRequest
{
    using Ptr = std::shared_ptr<SendEcdhCipherRequest>;
    std::string taskID;
    uint32_t offset;
    uint32_t size;
    std::vector<std::string> ecdhCiphers;

    SendEcdhCipherRequest() = default;
    ~SendEcdhCipherRequest() = default;

    explicit SendEcdhCipherRequest(const Json::Value& _params) { deserialize(_params); }

    void deserialize(const Json::Value& _params)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();

        if (!_params.isMember("offset"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the offset field"));
        }
        offset = _params["offset"].asUInt();

        if (!_params.isMember("size"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must define the size field"));
        }
        size = _params["size"].asUInt();

        if (!_params.isMember("ecdhCiphers"))
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int64_t)rpc::RpcError::InvalidRequest, "Must set the ecdhCiphers field"));
        }

        const Json::Value& ecdhCiphersJson = _params["ecdhCiphers"];
        if (!ecdhCiphersJson.isArray())
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "ecdhCiphers must be an array"));
        }

        for (const Json::Value& cipher : ecdhCiphersJson)
        {
            if (!cipher.isString() || cipher.asString().empty())
            {
                BOOST_THROW_EXCEPTION(BCOS_ERROR(
                    (int64_t)rpc::RpcError::InvalidRequest, "Invalid ecdhCipher format"));
            }
            ecdhCiphers.push_back(cipher.asString());
        }
    }
};

struct SendPartnerCipherRequest
{
    using Ptr = std::shared_ptr<SendPartnerCipherRequest>;
    std::string taskID;
    uint32_t offset{};
    uint32_t size{};
    uint32_t total{};
    std::vector<std::string> partnerCiphers;

    SendPartnerCipherRequest() = default;
    ~SendPartnerCipherRequest() = default;

    explicit SendPartnerCipherRequest(const Json::Value& _params) { deserialize(_params); }

    void deserialize(const Json::Value& _params)
    {
        if (!_params.isMember("taskID"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the taskID field"));
        }
        taskID = _params["taskID"].asString();

        if (!_params.isMember("offset"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must set the offset field"));
        }
        offset = _params["offset"].asUInt();

        if (!_params.isMember("size"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must define the size field"));
        }
        size = _params["size"].asUInt();

        if (!_params.isMember("total"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)rpc::RpcError::InvalidRequest, "Must define the total field"));
        }
        total = _params["total"].asUInt();

        if (!_params.isMember("partnerCiphers"))
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int64_t)rpc::RpcError::InvalidRequest, "Must set the partnerCiphers field"));
        }

        const Json::Value& partnerCiphersJson = _params["partnerCiphers"];
        if (!partnerCiphersJson.isArray())
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int64_t)rpc::RpcError::InvalidRequest, "partnerCiphers must be an array"));
        }

        for (const Json::Value& cipher : partnerCiphersJson)
        {
            if (!cipher.isString() || cipher.asString().empty())
            {
                BOOST_THROW_EXCEPTION(BCOS_ERROR(
                    (int64_t)rpc::RpcError::InvalidRequest, "Invalid partnerCiphers format"));
            }
            partnerCiphers.push_back(cipher.asString());
        }
    }
};

}  // namespace ppc::psi