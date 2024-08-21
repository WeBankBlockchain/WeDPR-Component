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
 * @file MPCService.h
 * @author: caryliao
 * @date 2023-03-24
 */
#pragma once
#include "Common.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include "ppc-rpc/src/RpcFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <bcos-utilities/Common.h>
#include <string>

using namespace bcos;
using namespace ppc;
using namespace ppc::io;
using namespace ppc::rpc;
using namespace ppc::tools;

namespace ppc::mpc
{
class MPCService
{
public:
    using Ptr = std::shared_ptr<MPCService>;
    MPCService() = default;
    virtual ~MPCService() = default;

    void runMpcRpc(Json::Value const& request, RespFunc func);
    void killMpcRpc(Json::Value const& request, RespFunc func);
    void setMPCConfig(MPCConfig const& mpcConfig);
    void setStorageConfig(StorageConfig const& storageConfig);

    JobInfo paramsToJobInfo(const Json::Value& params);
    void makeCommand(std::string& cmd, const JobInfo& jobInfo);
    void getMpcProtocol(const int participantCount, const bool isMalicious,
        std::string& mpcBinFileName, std::string& compileOption);
    void doRun(Json::Value const& request, Json::Value& response);
    void doKill(Json::Value const& request, Json::Value& response);
    void execCommand(const std::string cmd, int& outExitStatus, std::string& outResult);

    void writeStringToFile(const std::string& content, const std::string& filePath);
    void readAndSaveFile(LineReader::Ptr lineReader, LineWriter::Ptr lineWriter);
    LineReader::Ptr initialize_lineReader(const JobInfo& jobInfo, const std::string& readerFilePath,
        ppc::protocol::DataResourceType type);
    LineWriter::Ptr initialize_lineWriter(const JobInfo& jobInfo, const std::string& writerFilePath,
        ppc::protocol::DataResourceType type);

private:
    MPCConfig m_mpcConfig;
    StorageConfig m_storageConfig;
};
}  // namespace ppc::mpc
