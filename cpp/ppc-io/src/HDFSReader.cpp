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
 * @file HDFSReader.cpp
 * @author: yujiechen
 * @date 2022-11-30
 */
#include "HDFSReader.h"

using namespace bcos;
using namespace ppc::io;
using namespace ppc::storage;

HDFSReader::HDFSReader(FileHandler::Ptr _handler, uint64_t _bufferSize, char _lineSpliter)
  : BaseFileLineReader(_lineSpliter), m_handler(_handler), m_bufferSize(_bufferSize)
{
    m_length = ((hdfsFileInfo*)_handler->fileInfoHandler())->mSize;
    m_hostsInfo =
        hdfsGetHosts((hdfsFS)_handler->storageHandler(), _handler->path().c_str(), 0, m_length);
    if (!m_hostsInfo)
    {
        BOOST_THROW_EXCEPTION(
            HDFSOpenMetaInfoFailed()
            << errinfo_comment("hdfsGetHosts failed, error: " + std::string(hdfsGetLastError())));
    }
    // set the m_readGranularity
    m_readGranularity = m_bufferSize / 8;
    IO_LOG(DEBUG) << LOG_DESC("create HDFSReader success") << LOG_KV("path", _handler->path())
                  << LOG_KV("fileSize", m_length) << LOG_KV("bufferSize", m_bufferSize)
                  << LOG_KV("readGranularity", m_readGranularity);
}

bool HDFSReader::allocateCurrentBlock()
{
    if (readFinished())
    {
        return false;
    }
    // the loaded block-parser has not been read-finished
    if (m_currentBlockParser && m_currentBlockParser->pointer() < m_currentBlockParser->length())
    {
        return true;
    }

    // read data from the current fd
    if (m_currentFd && loadDataAndCreateBufferParser())
    {
        return true;
    }
    // the current fd is null or read-empty-data from the current fd
    if (!tryToOpenNewFile())
    {
        return false;
    }
    // open new file success, try to load-data and create buffer parser
    return loadDataAndCreateBufferParser();
}

bool HDFSReader::tryToOpenNewFile()
{
    m_currentBlockOffset++;
    if (!m_hostsInfo)
    {
        return false;
    }
    if (!m_hostsInfo[m_currentBlock])
    {
        return false;
    }
    char* hostName = m_hostsInfo[m_currentBlock][m_currentBlockOffset];
    // the block has been read-finished
    if (!hostName)
    {
        m_currentBlock++;
        // reset the offset to 0
        m_currentBlockOffset = 0;
    }
    // all blocks has been read-finished
    if (!m_hostsInfo[m_currentBlock])
    {
        IO_LOG(INFO) << LOG_DESC("tryToOpenNewFile finish for null block")
                     << LOG_KV("block", m_currentBlock);

        return false;
    }
    // open the file
    hostName = m_hostsInfo[m_currentBlock][m_currentBlockOffset];
    if (!hostName)
    {
        IO_LOG(INFO) << LOG_DESC("tryToOpenNewFile finish for null block items")
                     << LOG_KV("block", m_currentBlock)
                     << LOG_KV("blockOffset", m_currentBlockOffset);
        return false;
    }
    // release the old fd
    hdfsFS fs = (hdfsFS)m_handler->storageHandler();
    // try to open the new file
    m_currentFd.reset(new HdfsFileWrapper(
        m_handler, hdfsOpenFile2(fs, hostName, m_handler->path().c_str(), O_RDONLY,
                       ((hdfsFileInfo*)(m_handler->fileInfoHandler()))->mBlockSize, 0, 0)));
    if (!m_currentFd)
    {
        BOOST_THROW_EXCEPTION(OpenFileFailed() << errinfo_comment(
                                  "HDFSOpenFileFailed failed, host:" + std::string(hostName) +
                                  ", error: " + std::string(hdfsGetLastError())));
    }
    return true;
}