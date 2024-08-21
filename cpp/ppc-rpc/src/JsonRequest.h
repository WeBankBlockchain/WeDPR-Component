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
 * @file JsonRequest.h
 * @author: yujiechen
 * @date 2022-11-3
 */
#pragma once
#include "ppc-framework/rpc/RpcTypeDef.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <json/json.h>
#include <memory>

namespace ppc::rpc
{
class JsonRequest
{
public:
    using Ptr = std::shared_ptr<JsonRequest>;
    JsonRequest() = default;
    JsonRequest(std::string_view _data) { deserialize(_data); }

    virtual ~JsonRequest() {}

    virtual void setJsonRpc(std::string const& _jsonrpc) { m_jsonrpc = _jsonrpc; }
    virtual void setMethod(std::string const& _method) { m_method = _method; }
    virtual void setId(int64_t _id) { m_id = _id; }
    virtual void setParams(Json::Value const& _params) { m_params = _params; }

    virtual std::string const& jsonRpc() const { return m_jsonrpc; }
    virtual std::string const& token() const { return m_token; }
    virtual int64_t id() const { return m_id; }
    virtual Json::Value const& params() const { return m_params; }
    virtual std::string const& method() const { return m_method; }

protected:
    virtual void deserialize(std::string_view _requestBody)
    {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(_requestBody.begin(), _requestBody.end(), root))
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)RpcError::InvalidRequest,
                "invalid request for parse to json object failed"));
        }
        // parse jsonrpc field
        if (!root.isMember("jsonrpc"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)RpcError::InvalidRequest, "Must set the jsonrpc field"));
        }
        m_jsonrpc = root["jsonrpc"].asString();
        // parse jsonrpc field
        if (!root.isMember("token"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)RpcError::InvalidRequest, "Must set the token field"));
        }
        m_token = root["token"].asString();
        // parse method field
        if (!root.isMember("method"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)RpcError::InvalidRequest, "Must define the method field"));
        }
        m_method = root["method"].asString();
        // parse the id field, optional
        if (root.isMember("id"))
        {
            m_id = root["id"].asInt64();
        }
        // parse the params field
        if (!root.isMember("params"))
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)RpcError::InvalidRequest, "Must define the params field"));
        }
        m_params = root["params"];
    }

private:
    std::string m_jsonrpc;
    std::string m_token;
    std::string m_method;
    int64_t m_id;
    Json::Value m_params;
};
}  // namespace ppc::rpc