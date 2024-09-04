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
 * @file TestRpcRequestResponse.cpp
 * @author: yujiechen
 * @date 2022-11-4
 */
#include "ppc-rpc/src/JsonRequest.h"
#include "ppc-rpc/src/JsonResponse.h"
#include <bcos-utilities/Error.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::rpc;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(rpcRequestResponseTest, TestPromptFixture)
BOOST_AUTO_TEST_CASE(testJsonRequest)
{
    // case1: valid request
    std::string requestData =
        "{\"jsonrpc\":\"2.0\",\"token\":\"test\",\"method\":\"getBlockNumber\",\"params\":[1],"
        "\"id\":1}";
    auto jsonRequest = std::make_shared<JsonRequest>(requestData);
    BOOST_CHECK(jsonRequest->jsonRpc() == "2.0");
    BOOST_CHECK(jsonRequest->method() == "getBlockNumber");
    BOOST_CHECK(jsonRequest->id() == 1);
    Json::Value params = jsonRequest->params();
    BOOST_CHECK(params.isArray() == true);
    for (auto const& param : params)
    {
        BOOST_CHECK(param.asInt64() == 1);
    }
    // case2: invalid request without method
    requestData = "{\"jsonrpc\":\"2.0\",\"token\":\"test\",\"params\":[1],\"id\":1}";
    BOOST_CHECK_THROW(std::make_shared<JsonRequest>(requestData), bcos::Error);
    // case3: invalid json request
    requestData = "abc";
    BOOST_CHECK_THROW(std::make_shared<JsonRequest>(requestData), bcos::Error);

    // case4: valid request with taskInfo
    requestData =
        "{\"jsonrpc\":\"2.0\",\"token\":\"test\",\"method\":\"runTask\",\"params\":{\"algorithm\":"
        "45,\"params\":"
        "\"taskParam\", "
        "\"parties\":[{\"data\":{\"desc\":{\"path\":\"testPath\"},\"id\":\"dataSelf\",\"type\":0},"
        "\"desc\":\"selfParty\",\"id\":\"selfParty\",\"partyIndex\":0,\"type\":0}]},\"id\":1}";
    std::make_shared<JsonRequest>(requestData);
    BOOST_CHECK_NO_THROW(std::make_shared<JsonRequest>(requestData));
}

BOOST_AUTO_TEST_CASE(testJsonResponse)
{
    auto jsonResponse = std::make_shared<JsonResponse>();
    jsonResponse->setJsonRpc("3.0");
    jsonResponse->setId(10);
    BOOST_CHECK_NO_THROW(jsonResponse->serialize());
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test