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
 * @file Common.h
 * @author: caryliao
 * @date 2023-03-24
 */
#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/Log.h>

#define MPC_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("MPC")

namespace ppc::mpc
{
DERIVE_PPC_EXCEPTION(MpcCompilerNotExistException);
DERIVE_PPC_EXCEPTION(RunMpcFailException);
DERIVE_PPC_EXCEPTION(OpenPipeFailException);

struct JobInfo
{
    std::string jobId;
    bool mpcNodeUseGateway;
    std::string receiverNodeIp;
    int mpcNodeDirectPort;
    int participantCount;
    int selfIndex;
    bool isMalicious;
    int bitLength;
    std::string mpcFilePath;
    std::string inputFilePath;
    std::string outputFilePath;
    std::string gatewayEngineEndpoint;
};

const int MPC_SUCCESS = 0;
const int MPC_FAILED = -1;
const std::string PATH_SEPARATOR = "/";
const std::string MPC_RELATIVE_PATH = "/Programs/Source/";
const std::string MPC_ALGORITHM_FILE_SUFFIX = ".mpc";
const std::string MPC_ALGORITHM_COMPILER = "compile.py";
const std::string MPC_PREPARE_FILE = "mpc_prepare.csv";
const std::string MPC_RESULT_FILE = "mpc_result.csv";

enum MpcBinaryType
{
    Hemi,
    Replicated,
    Shamir,
    Mascot
};
const std::string MPC_BINARY_NAME[] = {
    "hemi-party.x", "replicated-ring-party.x", "shamir-party.x", "mascot-party.x"};

}  // namespace ppc::mpc
