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
 * @file FileLineReader.cpp
 * @author: yujiechen
 * @date 2022-10-19
 */

#include "FileLineReader.h"
#include "Common.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
using namespace ppc::io;
using namespace bcos;

FileLineReader::FileLineReader(
    std::string const& _filePath, int64_t _mmapGranularity, char _lineSpliter)
  : BaseFileLineReader(_lineSpliter),
    m_filePath(_filePath),
    c_pageSize(getpagesize()),
    m_mmapGranularity(_mmapGranularity)
{
    if (-1 == _mmapGranularity)
    {
        m_mmapGranularity = 500 * 1024 * c_pageSize;
    }
    // check the mmap-granularity
    if (m_mmapGranularity % c_pageSize != 0)
    {
        BOOST_THROW_EXCEPTION(
            InvalidMmapGranularity() << errinfo_comment(
                "the mmapGranularity must a multiple of page-size: " + std::to_string(c_pageSize)));
    }
    // check whether the file exists
    if (!boost::filesystem::exists(boost::filesystem::path(m_filePath)))
    {
        BOOST_THROW_EXCEPTION(OpenFileFailed() << errinfo_comment(_filePath + " not exist!"));
    }
    // the filePath should not be directory
    if (boost::filesystem::is_directory(boost::filesystem::path(m_filePath)))
    {
        BOOST_THROW_EXCEPTION(
            OpenFileFailed() << errinfo_comment(
                "The " + _filePath + " is a directory. Only support load content from file!"));
    }
    m_readGranularity = m_mmapGranularity / 8;
    // open the file
    m_fd = open(m_filePath.c_str(), O_RDONLY, 0644);
    if (-1 == m_fd)
    {
        BOOST_THROW_EXCEPTION(
            OpenFileFailed() << errinfo_comment("open file " + _filePath + " exception"));
    }
    m_length = lseek(m_fd, 0, SEEK_END);
    IO_LOG(INFO) << LOG_DESC("FileLineReader: open file success") << LOG_KV("file", _filePath)
                 << LOG_KV("length", m_length) << LOG_KV("pageSize", c_pageSize);
}

FileLineReader::~FileLineReader()
{
    if (-1 == m_fd)
    {
        return;
    }
    close(m_fd);
}

bool FileLineReader::allocateCurrentBlock()
{
    if (readFinished())
    {
        return false;
    }
    // holds mmap that has not been read-finished
    if (m_currentBlockParser && m_currentBlockParser->pointer() < m_currentBlockParser->length())
    {
        return true;
    }
    // without mmap or the mmap has already been read-finished, re-allocate a new mmapFile
    auto mmapSize = std::min(m_mmapGranularity, (m_length - m_offset));
    m_currentBlockParser = std::make_unique<MmapParser>(m_fd, m_offset, mmapSize);
    IO_LOG(TRACE) << LOG_DESC("allocate new mmap") << LOG_KV("size", mmapSize)
                  << LOG_KV("offset", m_offset);
    return true;
}