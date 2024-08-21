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
 * @file BaseFileLineReader.cpp
 * @author: yujiechen
 * @date 2022-10-19
 */

#include "BaseFileLineReader.h"
#include "Common.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
using namespace ppc::io;
using namespace bcos;
// get the next _size line data
DataBatch::Ptr BaseFileLineReader::next(int64_t _size, DataSchema schema)
{
    // the file has already been read-finished
    if (readFinished())
    {
        return nullptr;
    }
    switch (schema)
    {
    case DataSchema::String:
    {
        if (-1 == _size)
        {
            return readAllData<std::string>(DataSchema::String);
        }
        else if (_size <= 0)
        {
            BOOST_THROW_EXCEPTION(InvalidParam() << bcos::errinfo_comment(
                                      "The read-size parameter must be -1 or positive"));
        }
        else
        {
            return readLineData<std::string>(_size, DataSchema::String);
        }
    }
    case DataSchema::Bytes:
    {
        if (-1 == _size)
        {
            return readAllData<bcos::bytes>(DataSchema::Bytes);
        }
        else if (_size <= 0)
        {
            BOOST_THROW_EXCEPTION(InvalidParam() << bcos::errinfo_comment(
                                      "The read-size parameter must be -1 or positive"));
        }
        else
        {
            return readLineData<bcos::bytes>(_size, DataSchema::Bytes);
        }
    }
    default:
    {
        BOOST_THROW_EXCEPTION(
            UnsupportedDataSchema() << errinfo_comment("unsupported data schema"));
    }
    }
}

// read all bytes from the file, without parse
bcos::bytes BaseFileLineReader::readBytes()
{
    bcos::bytes result;
    while (allocateCurrentBlock())
    {
        m_currentBlockParser->readAll(result);
        m_offset = result.size();
    }
    IO_LOG(INFO) << LOG_DESC("readBytes") << LOG_KV("result", result.size())
                 << LOG_KV("length", m_length);
    return result;
}