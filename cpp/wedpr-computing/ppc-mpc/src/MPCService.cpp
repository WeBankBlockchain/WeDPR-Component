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
 * @file MPCService.cpp
 * @author: caryliao
 * @date 2023-03-28
 */

#include "MPCService.h"
#include "Common.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-io/src/DataResourceLoaderImpl.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include "ppc-storage/src/FileStorageFactoryImpl.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <stdlib.h>
#include <tbb/parallel_for.h>
#include <boost/filesystem/operations.hpp>
#include <fstream>
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using namespace ppc;
using namespace ppc::mpc;
using namespace bcos;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::tools;
using namespace ppc::storage;
using namespace ppc::rpc;

void MPCService::removeAllFiles(const std::vector<std::string> &files)
{
    for (const auto &file : files)
    {
        if (file.empty())
        {
            continue;
        }
        try {
            if (boost::filesystem::exists(file))
            {
                boost::filesystem::remove_all(file);

                MPC_LOG(INFO) << LOG_DESC("[MPCService][removeAllFiles]")
                      << LOG_KV("file", file);
            }
        } catch (...) {
            MPC_LOG(INFO) << LOG_DESC("[MPCService][removeAllFiles]")
                      << LOG_DESC("remove file exception")
                      << LOG_KV("file", file);
        }
    }
}

void MPCService::doRun(Json::Value const& request, Json::Value& response)
{
    auto startT = utcSteadyTime();

    std::string localPathPrefix;
    std::string mpcFileLocalPath;
    try
    {  // 0 get jobInfo and make command
        auto jobInfo = paramsToJobInfo(request);

        std::string jobId = jobInfo.jobId;
        int participantCount = jobInfo.participantCount;
        int selfIndex = jobInfo.selfIndex;

        std::string mpcCmd;
        makeCommand(mpcCmd, jobInfo);

        localPathPrefix =
            m_mpcConfig.jobPath + PATH_SEPARATOR + jobInfo.jobId + PATH_SEPARATOR;

        // 1 download mpc algorithm file
        std::string mpcFileHdfsPath = jobInfo.mpcFilePath;
        std::string mpcRootPath = m_mpcConfig.mpcRootPathNoGateway;
        if (jobInfo.mpcNodeUseGateway)
        {
            mpcRootPath = m_mpcConfig.mpcRootPath;
        }
        
        mpcFileLocalPath =
            mpcRootPath + MPC_RELATIVE_PATH + jobInfo.jobId + MPC_ALGORITHM_FILE_SUFFIX;

        auto mpcFileReader =
                initialize_lineReader(jobInfo, mpcFileHdfsPath, DataResourceType::HDFS);
        auto mpcFileWriter =
                initialize_lineWriter(jobInfo, mpcFileLocalPath, DataResourceType::FILE);
            readAndSaveFile(mpcFileHdfsPath, mpcFileLocalPath, mpcFileReader, mpcFileWriter);


        // 2 download mpc prepare file
        std::string mpcPrepareFileHdfsPath = jobInfo.inputFilePath;
        
        // std::string inputFilePath =
        // m_mpcConfig.jobPath + PATH_SEPARATOR + jobInfo.jobId + PATH_SEPARATOR + MPC_PREPARE_FILE;

        std::string mpcPrepareFileLocalPath = localPathPrefix + MPC_PREPARE_FILE + "-P" + std::to_string(selfIndex) + "-0";
        auto datasetFileReader =
                initialize_lineReader(jobInfo, mpcPrepareFileHdfsPath, DataResourceType::HDFS);
            auto datasetFileWriter =
                initialize_lineWriter(jobInfo, mpcPrepareFileLocalPath, DataResourceType::FILE);
            readAndSaveFile(mpcPrepareFileHdfsPath, mpcPrepareFileLocalPath, datasetFileReader, datasetFileWriter);

        // 3 run mpc job
        int outExitStatus = MPC_SUCCESS;
        std::string outResult;
        execCommand(mpcCmd, outExitStatus, outResult);

        if (outExitStatus != MPC_SUCCESS)
        {
            removeAllFiles(std::vector<std::string>{localPathPrefix, mpcFileLocalPath});
            MPC_LOG(ERROR) << LOG_DESC("[MPCService][doRun]") 
                << "run mpc job failed"
                << LOG_KV("jobId", jobId)
                << LOG_KV("outExitStatus", outExitStatus)
                << LOG_KV("outResult", outResult);
            BOOST_THROW_EXCEPTION(RunMpcFailException() << errinfo_comment(outResult));
        }
        
        std::string message = "run mpc job successfully";
        MPC_LOG(INFO) << LOG_DESC("[MPCService][doRun]") << LOG_KV("jobId", jobId) << LOG_DESC(message);
        // MPC_LOG(DEBUG) << LOG_DESC("[MPCService][doRun]") << LOG_KV("jobId", jobId) << LOG_KV("outResult", outResult);
        response["code"] = MPC_SUCCESS;
        response["message"] = "success";

        // 4 upload result file
        std::string resultFileHdfsPath =  jobInfo.outputFilePath;
        std::string resultFileLocalPath = localPathPrefix + MPC_RESULT_FILE;
        writeStringToFile(outResult, resultFileLocalPath);

        auto resultFileReader =
            initialize_lineReader(jobInfo, resultFileLocalPath, DataResourceType::FILE);
        auto resultFileWriter =
            initialize_lineWriter(jobInfo, resultFileHdfsPath, DataResourceType::HDFS);
        readAndSaveFile(resultFileLocalPath, resultFileHdfsPath, resultFileReader, resultFileWriter);

        removeAllFiles(std::vector<std::string>{localPathPrefix, mpcFileLocalPath});
    }
    catch (const std::exception& e)
    {
        removeAllFiles(std::vector<std::string>{localPathPrefix, mpcFileLocalPath});

        const std::string diagnostic_information = std::string(boost::diagnostic_information(e));
        MPC_LOG(INFO) << LOG_DESC("[MPCService][doRun]") << LOG_DESC("run mpc job failed")
                      << LOG_DESC(diagnostic_information);
        response["code"] = MPC_FAILED;
        response["message"] = diagnostic_information;
    }

    MPC_LOG(INFO) << LOG_DESC("run mpc") << LOG_KV("request",  request.toStyledString())<< LOG_KV("timecost(ms)", utcSteadyTime() - startT);
}

void MPCService::runMpcRpc(Json::Value const& request, RespFunc func)
{
    Json::Value response;
    doRun(request, response);
    func(nullptr, std::move(response));
}

void MPCService::doKill(Json::Value const& request, Json::Value& response)
{
    auto startT = utcSteadyTime();
    try
    {
        auto jobId = request["jobId"].asString();
        MPC_LOG(INFO) << LOG_DESC("[MPCService][doKill]") << LOG_KV("jobId", jobId);

        std::string killCmd = "ps -ef |grep mpc | grep " + jobId +
                              " | grep -v grep | awk '{print $2}' | xargs kill -9";
        MPC_LOG(INFO) << LOG_DESC("[MPCService][doKill]") << LOG_KV("killCmd", killCmd);

        int outExitStatus = 0;
        std::string outResult;
        execCommand(killCmd, outExitStatus, outResult);
        std::string message = "";
        if (outExitStatus == 0)
        {
            message = "Kill mpc job successfully";
            MPC_LOG(INFO) << LOG_DESC("[MPCService][doKill]") << LOG_DESC(message);
            response["code"] = MPC_SUCCESS;
            response["message"] = "success";
        }
        else
        {
            message = "Kill mpc job failed";
            MPC_LOG(INFO) << LOG_DESC("[MPCService][doKill]") << LOG_DESC(message);
            response["code"] = MPC_FAILED;
            response["message"] = message;
        }
    }
    catch (const std::exception& e)
    {
        const std::string diagnostic_information = std::string(boost::diagnostic_information(e));
        MPC_LOG(INFO) << LOG_DESC("[MPCService][doKill]") << LOG_DESC("kill mpc job failed:")
                      << LOG_DESC(diagnostic_information);
        response["code"] = MPC_FAILED;
        response["message"] = diagnostic_information;
    }
    MPC_LOG(INFO) << LOG_DESC("kill mpc job") << LOG_KV("timecost(ms)", utcSteadyTime() - startT);
}

void MPCService::killMpcRpc(Json::Value const& request, RespFunc func)
{
    Json::Value response;
    doKill(request, response);
    func(nullptr, std::move(response));
}

void MPCService::writeStringToFile(const std::string& content, const std::string& filePath)
{
    std::ostringstream buffer;
    buffer << content;

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    file << buffer.str();
}

void MPCService::readAndSaveFile(const std::string &readerFilePath, const std::string &writerFilePath, LineReader::Ptr lineReader, LineWriter::Ptr lineWriter)
{
    uint64_t lineSize = 0;
    int64_t readPerBatchLines = m_mpcConfig.readPerBatchLines;
    while (true)
    {
        // batch read dataset line
        auto dataBatch = lineReader->next(readPerBatchLines);
        if (!dataBatch)
        {
            break;
        }
        lineSize += dataBatch->size();
        lineWriter->writeLine(dataBatch, DataSchema::String, "\n");
    }
    lineWriter->close();
    MPC_LOG(INFO) << LOG_DESC("save file ok") 
        << LOG_KV("readerFilePath", readerFilePath)
        << LOG_KV("writerFilePath", writerFilePath)
        << LOG_KV("file lines", lineSize);
}

LineReader::Ptr MPCService::initialize_lineReader(
    const JobInfo& jobInfo, const std::string& readerFilePath, DataResourceType type)
{
    auto factory = std::make_shared<FileStorageFactoryImpl>();
    auto dataResourceLoader = std::make_shared<DataResourceLoaderImpl>(
        nullptr, nullptr, nullptr, nullptr, factory, nullptr);
    const auto dataResource = std::make_shared<DataResource>();
    dataResource->setResourceID(jobInfo.jobId);
    const auto dataResourceDesc = std::make_shared<DataResourceDesc>();
    dataResourceDesc->setType((int)type);
    if ((int)type == (int)DataResourceType::HDFS)
    {
        dataResourceDesc->setFileStorageConnectOption(m_storageConfig.fileStorageConnectionOpt);
    }
    dataResourceDesc->setPath(readerFilePath);
    dataResource->setDesc(dataResourceDesc);
    return dataResourceLoader->loadReader(dataResource->desc());
}

LineWriter::Ptr MPCService::initialize_lineWriter(
    const JobInfo& jobInfo, const std::string& writerFilePath, DataResourceType type)
{
    auto factory = std::make_shared<FileStorageFactoryImpl>();
    auto dataResourceLoader = std::make_shared<DataResourceLoaderImpl>(
        nullptr, nullptr, nullptr, nullptr, factory, nullptr);
    const auto dataResource = std::make_shared<DataResource>();
    dataResource->setResourceID(jobInfo.jobId);
    const auto dataResourceDesc = std::make_shared<DataResourceDesc>();
    dataResourceDesc->setType((int)type);

    if ((int)type == (int)DataResourceType::HDFS)
    {
        dataResourceDesc->setFileStorageConnectOption(m_storageConfig.fileStorageConnectionOpt);
    }
    dataResourceDesc->setPath(writerFilePath);
    dataResource->setOutputDesc(dataResourceDesc);
    return dataResourceLoader->loadWriter(dataResource->outputDesc());
}


void MPCService::setMPCConfig(MPCConfig const& mpcConfig)
{
    m_mpcConfig = mpcConfig;
}

void MPCService::setStorageConfig(StorageConfig const& storageConfig)
{
    m_storageConfig = storageConfig;
}

JobInfo MPCService::paramsToJobInfo(const Json::Value& params)
{
    try
    {
        JobInfo jobInfo;
        jobInfo.jobId = params["jobId"].asString();
        jobInfo.mpcNodeUseGateway = params["mpcNodeUseGateway"].asBool();
        jobInfo.receiverNodeIp = params["receiverNodeIp"].asString();
        jobInfo.mpcNodeDirectPort = params["mpcNodeDirectPort"].asInt();
        jobInfo.participantCount = params["participantCount"].asInt();
        jobInfo.selfIndex = params["selfIndex"].asInt();
        jobInfo.isMalicious = params["isMalicious"].asBool();
        jobInfo.bitLength = params["bitLength"].asInt();
        jobInfo.mpcFilePath = params["mpcFilePath"].asString();
        jobInfo.inputFilePath = params["inputFilePath"].asString();
        jobInfo.outputFilePath = params["outputFilePath"].asString();
        jobInfo.gatewayEngineEndpoint = params["gatewayEngineEndpoint"].asString();
        return jobInfo;
    }
    catch (const std::exception& e)
    {
        BOOST_THROW_EXCEPTION(
            InvalidParam() << errinfo_comment(
                "invalid params:" + std::string(boost::diagnostic_information(e))));
    }
}

void MPCService::makeCommand(std::string& cmd, const JobInfo& jobInfo)
{
    std::string jobId = jobInfo.jobId;
    std::string mpcRootPath = m_mpcConfig.mpcRootPath;
    if (jobInfo.mpcNodeUseGateway)
    {
        MPC_LOG(INFO) << LOG_DESC("[MPCService][makeCommand] use gateway to connect node") 
            << LOG_KV("jobId", jobId);
    }
    else
    {
        mpcRootPath = m_mpcConfig.mpcRootPathNoGateway;
        MPC_LOG(INFO) << LOG_DESC("[MPCService][makeCommand] direct connect node")
            << LOG_KV("jobId", jobId);
    }
    int r = chdir(mpcRootPath.c_str());
    if (r == 0)
    {
        MPC_LOG(INFO) << LOG_DESC("[MPCService][makeCommand] change dir ok")
            << LOG_KV("jobId", jobId);
    }
    else
    {
        MPC_LOG(ERROR) << LOG_DESC("[MPCService][makeCommand] change dir fail") 
            << LOG_KV("mpcRootPath", mpcRootPath)
            << LOG_KV("ret", r)
            << LOG_KV("jobId", jobId);
            ;
    }
    std::string compileFilePath = mpcRootPath + PATH_SEPARATOR + MPC_ALGORITHM_COMPILER;
    int participantCount = jobInfo.participantCount;
    int selfIndex = jobInfo.selfIndex;
    bool isMalicious = jobInfo.isMalicious;
    std::string mpcBinFileName;
    std::string compileOption;
    getMpcProtocol(participantCount, isMalicious, mpcBinFileName, compileOption);

    if (!boost::filesystem::exists(compileFilePath))
    {
        MPC_LOG(ERROR) << LOG_DESC("[MPCService] compile file not exist") 
            << LOG_KV("compileFilePath", compileFilePath)
            << LOG_KV("jobId", jobId);
            ;

        BOOST_THROW_EXCEPTION(MpcCompilerNotExistException()
                              << errinfo_comment("compile file not exist:" + compileFilePath));
    }
    if (jobInfo.mpcNodeUseGateway)
    {
        cmd = "export LD_LIBRARY_PATH=$PPC_MPC_LIB && python ";
    }
    else
    {
        cmd = "export LD_LIBRARY_PATH=$PPC_MPC_NO_GATEWAY_LIB && python ";
    }
    std::string compileCmd = compileFilePath + " " + compileOption + +" " +
                             std::to_string(jobInfo.bitLength) + " " + jobInfo.jobId;
    cmd += compileCmd + " && ";
    cmd += mpcRootPath + PATH_SEPARATOR + mpcBinFileName + " ";
    cmd += std::to_string(jobInfo.selfIndex) + " ";
    cmd += jobInfo.jobId + " ";
    if (jobInfo.mpcNodeUseGateway)
    {
        cmd += "-gateway " + jobInfo.gatewayEngineEndpoint + " ";
        cmd += "-ID " + jobInfo.jobId.substr(jobInfo.jobId.length() - 8, 8) + " ";
    }
    else
    {
        cmd += "-h " + jobInfo.receiverNodeIp + " ";
        cmd += "-pn " + std::to_string(jobInfo.mpcNodeDirectPort) + " ";
    }
    if (jobInfo.mpcNodeUseGateway && !jobInfo.isMalicious && jobInfo.participantCount >= 3)
    {
        cmd += "-u ";
    }
    std::string inputFilePath =
        m_mpcConfig.jobPath + PATH_SEPARATOR + jobInfo.jobId + PATH_SEPARATOR + MPC_PREPARE_FILE;
    cmd += "-IF " + inputFilePath + " ";
    if (mpcBinFileName != MPC_BINARY_NAME[MpcBinaryType::Replicated])
    {
        cmd += "-N " + std::to_string(jobInfo.participantCount) + " ";
    }
    MPC_LOG(INFO) << LOG_DESC("[MPCService][makeCommand]") << LOG_KV("jobId", jobId) << LOG_KV("mpcCmd", cmd);
}

void MPCService::getMpcProtocol(const int participantCount, const bool isMalicious,
    std::string& mpcBinFileName, std::string& compileOption)
{
    if (participantCount <= 1)
    {
        BOOST_THROW_EXCEPTION(
            InvalidParam() << errinfo_comment("getMpcProtocol: participantCount need at least 2"));
    }
    if (isMalicious == true)
    {
        mpcBinFileName = MPC_BINARY_NAME[MpcBinaryType::Mascot];
        compileOption = "-F";
    }
    else
    {
        if (participantCount == 2)
        {
            mpcBinFileName = MPC_BINARY_NAME[MpcBinaryType::Hemi];
            compileOption = "-F";
        }
        else if (participantCount == 3)
        {
            mpcBinFileName = MPC_BINARY_NAME[MpcBinaryType::Replicated];
            compileOption = "-R";
        }
        else
        {
            mpcBinFileName = MPC_BINARY_NAME[MpcBinaryType::Shamir];
            compileOption = "-F";
        }
    }
}

void MPCService::execCommand(const std::string cmd, int& outExitStatus, std::string& outResult)
{
    try
    {
        auto pPipe = popen((cmd + " 2>&1").c_str(), "r");
        if (pPipe == nullptr)
        {
            BOOST_THROW_EXCEPTION(OpenPipeFailException() << errinfo_comment("open pipe fail"));
        }
        std::array<char, 256> buffer;
        while (not std::feof(pPipe))
        {
            auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
            outResult.append(buffer.data(), bytes);
        }
        auto rc = pclose(pPipe);
        if (WIFEXITED(rc))
        {
            outExitStatus = WEXITSTATUS(rc);
        }
        MPC_LOG(INFO) << LOG_DESC("[MPCService][execCommand]")
                      << LOG_KV("outExitStatus", outExitStatus);
    }
    catch (const std::exception& e)
    {
        MPC_LOG(WARNING) << LOG_DESC("[MPCService] execCommand failed") << LOG_KV("cmd", cmd);
        BOOST_THROW_EXCEPTION(
            RunMpcFailException() << errinfo_comment(
                "invalid params:" + std::string(boost::diagnostic_information(e))));
    }
}
