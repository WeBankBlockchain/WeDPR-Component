/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file TestmpcService->cpp
 * @author: caryliao
 * @date 2023-03-28
 */
#include "ppc-mpc/src/Common.h"
#include "ppc-mpc/src/MPCService.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::mpc;
using namespace bcos;
using namespace bcos::test;
using namespace ppc::io;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(MPCServiceTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(testMPCService)
{
    auto mpcService = std::make_shared<MPCService>();
    std::string configPath{"../../../../wedpr-computing/ppc-mpc/tests/data/config.ini"};
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(configPath, pt);
    // load the mpc config
    auto ppcConfig = std::make_shared<PPCConfig>();
    ppcConfig->loadMPCConfig(pt);
    auto mpcConfig = ppcConfig->mpcConfig();
    auto storageConfig = ppcConfig->storageConfig();
    mpcService->setMPCConfig(mpcConfig);
    mpcService->setStorageConfig(storageConfig);

    // test mpc config
    BOOST_CHECK(mpcConfig.datasetHDFSPath == "/user/ppc/");
    BOOST_CHECK(mpcConfig.jobPath == "/data/app/ppc/mpc-job/");
    BOOST_CHECK(mpcConfig.mpcRootPath == "/ppc/scripts/ppc-mpc/");
    BOOST_CHECK(mpcConfig.mpcRootPathNoGateway == "/ppc/scripts/ppc-mpc-no-gateway/");
    BOOST_CHECK(mpcConfig.readPerBatchLines == 100000);

    // test getMpcProtocol
    std::string mpcBinFileName, compileOption;
    BOOST_CHECK_THROW(
        mpcService->getMpcProtocol(1, false, mpcBinFileName, compileOption), InvalidParam);

    mpcService->getMpcProtocol(2, true, mpcBinFileName, compileOption);
    BOOST_CHECK_EQUAL(mpcBinFileName, MPC_BINARY_NAME[MpcBinaryType::Mascot]);
    BOOST_CHECK_EQUAL(compileOption, "-F");

    mpcService->getMpcProtocol(2, false, mpcBinFileName, compileOption);
    BOOST_CHECK_EQUAL(mpcBinFileName, MPC_BINARY_NAME[MpcBinaryType::Hemi]);
    BOOST_CHECK_EQUAL(compileOption, "-F");

    mpcService->getMpcProtocol(3, false, mpcBinFileName, compileOption);
    BOOST_CHECK_EQUAL(mpcBinFileName, MPC_BINARY_NAME[MpcBinaryType::Replicated]);
    BOOST_CHECK_EQUAL(compileOption, "-R");

    mpcService->getMpcProtocol(4, false, mpcBinFileName, compileOption);
    BOOST_CHECK_EQUAL(mpcBinFileName, MPC_BINARY_NAME[MpcBinaryType::Shamir]);
    BOOST_CHECK_EQUAL(compileOption, "-F");

    // test makeCommand
    std::string cmd;
    JobInfo jobInfo;
    jobInfo.jobId = "j-123456789";
    jobInfo.mpcNodeUseGateway = true;
    jobInfo.receiverNodeIp = "";
    jobInfo.mpcNodeDirectPort = 5899;
    jobInfo.participantCount = 2;
    jobInfo.selfIndex = 0;
    jobInfo.isMalicious = false;
    jobInfo.bitLength = 128;
    jobInfo.inputFilePath = "mpc_prepare.csv";
    jobInfo.outputFilePath = "mpc_output.txt";
    jobInfo.gatewayEngineEndpoint = "127.0.0.1:6789";

    mpcService->makeCommand(cmd, jobInfo);
    BOOST_CHECK_EQUAL(cmd,
        "/ppc/scripts/ppc-mpc//compile.py -F 128 j-123456789 && /ppc/scripts/ppc-mpc//hemi-party.x "
        "0 j-123456789 -gateway 127.0.0.1:6789 -ID 23456789 -IF "
        "/data/app/ppc/mpc-job//j-123456789/mpc_prepare.csv -N 2 ");


    jobInfo.participantCount = 3;
    mpcService->makeCommand(cmd, jobInfo);
    BOOST_CHECK_EQUAL(cmd,
        "/ppc/scripts/ppc-mpc//compile.py -R 128 j-123456789 && "
        "/ppc/scripts/ppc-mpc//replicated-ring-party.x "
        "0 j-123456789 -gateway 127.0.0.1:6789 -ID 23456789 -u -IF "
        "/data/app/ppc/mpc-job//j-123456789/mpc_prepare.csv ");

    jobInfo.participantCount = 4;
    mpcService->makeCommand(cmd, jobInfo);
    BOOST_CHECK_EQUAL(cmd,
        "/ppc/scripts/ppc-mpc//compile.py -F 128 j-123456789 && "
        "/ppc/scripts/ppc-mpc//shamir-party.x 0 j-123456789 -gateway 127.0.0.1:6789 -ID 23456789 "
        "-u -IF /data/app/ppc/mpc-job//j-123456789/mpc_prepare.csv -N 4 ");

    jobInfo.participantCount = 4;
    jobInfo.isMalicious = true;
    jobInfo.mpcNodeUseGateway = false;
    jobInfo.receiverNodeIp = "192.168.1.2";
    mpcService->makeCommand(cmd, jobInfo);
    BOOST_CHECK_EQUAL(cmd,
        "/ppc/scripts/ppc-mpc-no-gateway//compile.py -F 128 j-123456789 && "
        "/ppc/scripts/ppc-mpc-no-gateway//mascot-party.x 0 j-123456789 -h 192.168.1.2 -pn 5899 "
        "-IF /data/app/ppc/mpc-job//j-123456789/mpc_prepare.csv -N 4 ");
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
