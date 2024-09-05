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
 * @author: asherli
 * @date 2023-03-13
 */
#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <err.h>
#include <fcntl.h>
#include <json/json.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/exception/diagnostic_information.hpp>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define PIR_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PIR")

namespace ppc::pir
{
// const std::string DEFAULT_MESSAGE = "message not found";
const std::string DEFAULT_PREFIX = "PPC_DEFAULT_MESG";


enum class OTPIRMessageType : uint8_t
{
    HELLO_RECEIVER = 0x01,
    RESULTS = 0x02
};

enum class OTPIRRetCode : int
{
    ON_EXCEPTION = -3000,
    UNDEFINED_TASK_ROLE = -3001,
    INVALID_TASK_PARAM = -3002
};

inline bool isPrefixMatched(const bcos::bytes& _prefix, const char* begin, const char* end)
{
    size_t prefixLen = _prefix.size();
    if (end - begin < prefixLen)
        return false;
    for (size_t i = 0; i < prefixLen; ++i)
    {
        if (_prefix[i] != *(begin + i))
            return false;
    }
    return true;
}

// 从Task param解析
struct PirTaskMessage
{
    std::string searchId;
    std::string requestAgencyId;
    std::string requestAgencyDataset;
    uint32_t prefixLength;
};

// 通过前缀读取符合要求的行，写到buffer中
// 我们约定id在每行最前
inline std::vector<std::pair<bcos::bytes, bcos::bytes>> readInputsByMmapWithPrefix(
    const std::string& _path, const bcos::bytes& _prefix)
{
    std::vector<std::pair<bcos::bytes, bcos::bytes>> _buffer;
    int fd = open(_path.c_str(), O_RDONLY);
    struct stat fs
    {
    };

    if (fd == -1 || fstat(fd, &fs) == -1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("open file error: " + _path));
    }

    char* buf = (char*)mmap(nullptr, fs.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED)
    {
        close(fd);
        BOOST_THROW_EXCEPTION(std::runtime_error("mmap file to memory error: " + _path));
    }

    char* buf_end = buf + fs.st_size;

    char *begin = buf, *end = buf, c;
    while (true)
    {
        if (!(*end == '\r' || *end == '\n'))
        {
            if (++end < buf_end)
                continue;
        }
        else if (1 + end < buf_end)
        {
            // see if we got "\r\n" or "\n\r" here
            c = *(1 + end);
            if ((c == '\r' || c == '\n') && c != *end)
                ++end;
        }
        // 判断_prefix和buf中begin, end这一行的前缀是否相同
        if (isPrefixMatched(_prefix, begin, end))
        {
            std::string st(begin, end);
            std::stringstream ss(st);
            std::string idIndex;
            std::getline(ss, idIndex, ',');
            // 将begin, end这一行的前缀是否相同按逗号分割，取逗号前的内容
            std::pair<bcos::bytes, bcos::bytes> pairTmp;
            pairTmp.first = bcos::bytes(idIndex.begin(), idIndex.end());
            pairTmp.second = bcos::bytes(begin, end);
            _buffer.push_back(pairTmp);
            // _buffer[bcos::bytes(idIndex.begin(), idIndex.end())] = bcos::bytes(begin, end);
            // _buffer->push_back(bcos::bytes(begin, end));
        }

        if ((begin = ++end) >= buf_end)
            break;
    }

    munmap(buf, fs.st_size);
    close(fd);
    return _buffer;
}

inline PirTaskMessage parseJson(std::string_view _param)
{
    PIR_LOG(TRACE) << LOG_BADGE("parseJson") LOG_KV("_param", _param);
    Json::Reader reader;
    Json::Value result;
    if (!reader.parse(_param.begin(), _param.end(), result))
    {
        BOOST_THROW_EXCEPTION(
            BCOS_ERROR((int)OTPIRRetCode::INVALID_TASK_PARAM, "invalid task param: invalid json"));
    }
    PIR_LOG(TRACE) << LOG_BADGE("result type") LOG_KV("result", result.type());
    if (!result.isObject() || result.empty())
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int)OTPIRRetCode::INVALID_TASK_PARAM,
            "invalid task param:: the param must be object"));
    }
    PirTaskMessage taskMessage;

    if (!result.isMember("searchId"))
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int)OTPIRRetCode::INVALID_TASK_PARAM,
            "invalid task param:: the param searchId not found"));
    }
    if (!result.isMember("requestAgencyId"))
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int)OTPIRRetCode::INVALID_TASK_PARAM,
            "invalid task param:: the param requestAgencyId not found"));
    }
    if (!result.isMember("prefixLength"))
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int)OTPIRRetCode::INVALID_TASK_PARAM,
            "invalid task param:: the param prefixLength not found"));
    }
    // if(!result.isMember("requestAgencyDataset")){
    //     BOOST_THROW_EXCEPTION(BCOS_ERROR((int)OTPIRRetCode::INVALID_TASK_PARAM,
    //         "invalid task param:: the param requestAgencyDataset not found"));
    // }
    taskMessage.searchId = result["searchId"].asString();
    taskMessage.requestAgencyId = result["requestAgencyId"].asString();
    // taskMessage.requestAgencyDataset = result["requestAgencyDataset"].asString();
    taskMessage.prefixLength = result.get("prefixLength", 0).asInt();
    // taskMessage.prefixLength = result['prefixLength'].asInt();
    // if(result.isMember("requestAgencyDataset")){
    //     taskMessage.requestAgencyDataset = result["requestAgencyDataset"].asString();
    // }
    return taskMessage;
}


DERIVE_PPC_EXCEPTION(CipherLengthFailException);
DERIVE_PPC_EXCEPTION(FIleDeleteFailException);
}  // namespace ppc::pir