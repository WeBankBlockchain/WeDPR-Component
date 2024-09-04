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
 * @file JsonResponse.h
 * @author: yujiechen
 * @date 2022-11-3
 */
#pragma once
#include <bcos-utilities/Error.h>
#include <json/json.h>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <memory>

namespace ppc::rpc
{
class JsonSink
{
public:
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;

    JsonSink(bcos::bytes& buffer) : m_buffer(buffer) {}

    std::streamsize write(const char* s, std::streamsize n)
    {
        m_buffer.insert(m_buffer.end(), (bcos::byte*)s, (bcos::byte*)s + n);
        return n;
    }

    bcos::bytes& m_buffer;
};
class JsonResponse
{
public:
    using Ptr = std::shared_ptr<JsonResponse>;
    JsonResponse() : m_error(std::make_shared<bcos::Error>(0, "success")) {}
    virtual ~JsonResponse() = default;

    void setJsonRpc(std::string const& _jsonrpc) { m_jsonrpc = _jsonrpc; }
    void setId(int64_t _id) { m_id = _id; }
    void setError(bcos::Error const& _error) { *m_error = _error; }
    void setError(bcos::Error&& _error) { *m_error = std::move(_error); }
    void setResult(Json::Value&& _result) { m_result = std::move(_result); }

    bcos::Error::Ptr mutableError() { return m_error; }

    virtual bcos::bytes serialize()
    {
        Json::Value response;
        response["jsonrpc"] = m_jsonrpc;
        response["id"] = m_id;
        if (m_error && m_error->errorCode())
        {
            m_result["code"] = m_error->errorCode();
            m_result["message"] = m_error->errorMessage();
        }
        response["result"] = m_result;
        bcos::bytes out;
        boost::iostreams::stream<JsonSink> outputStream(out);
        std::unique_ptr<Json::StreamWriter> writer(Json::StreamWriterBuilder().newStreamWriter());
        writer->write(response, &outputStream);
        writer.reset();
        return out;
    }

private:
    std::string m_jsonrpc;
    int64_t m_id;
    bcos::Error::Ptr m_error;
    Json::Value m_result;
};
}  // namespace ppc::rpc