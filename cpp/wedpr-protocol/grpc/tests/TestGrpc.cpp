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
 * @file TestGrpc.cpp
 * @author: yujiechen
 * @date 2022-11-12
 */

#include "wedpr-protocol/grpc/Common.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::protocol;
using namespace bcos::test;
using namespace grpc;

BOOST_FIXTURE_TEST_SUITE(GrpcTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(testGrpc)
{
    grpc::ChannelArguments args;
    uint64_t msgSize = INT_MAX;
    int convertedMsgSize = msgSize;
    std::cout << "compare with -1, result: " << (convertedMsgSize >= -1) << std::endl;
    args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, INT_MAX);
    std::cout << "##### " << GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH << ", " << INT_MAX << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()