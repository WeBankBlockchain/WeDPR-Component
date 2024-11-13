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
 * @file HDFSReader.h
 * @author: yujiechen
 * @date 2022-11-30
 */
#pragma once
#include "BaseFileLineReader.h"
#include "parser/BufferParser.h"
#include "ppc-framework/io/LineReader.h"
#include "ppc-framework/storage/FileStorage.h"
#include <hdfs/hdfs.h>

namespace ppc::io
{
class HDFSReader : public BaseFileLineReader
{
public:
    using Ptr = std::shared_ptr<HDFSReader>;
    HDFSReader(ppc::storage::FileHandler::Ptr _handler, uint64_t _bufferSize = 500 * 1024 * 1024,
        char _lineSpliter = '\n');
    ~HDFSReader() override = default;

    // the capacity of the file or memory-bytes
    uint64_t capacity() const override { return m_length; }
    ppc::protocol::DataResourceType type() const override
    {
        return ppc::protocol::DataResourceType::HDFS;
    }

    bool readFinished() const override { return (m_offset == m_length); }

protected:
    bool allocateCurrentBlock() override;
    virtual bool tryToOpenNewFile();

    inline bool loadDataAndCreateBufferParser()
    {
        auto buffer = std::make_shared<bcos::bytes>();
        auto dataLen = std::min(m_bufferSize, (m_length - m_offset));
        buffer->resize(dataLen);

        hdfsFS fs = (hdfsFS)m_handler->storageHandler();
        // read data from the hdfs
        uint64_t readedCount = 0;
        int64_t readedBytes = 0;
        // Note: when read-finished, hdfsRead return 0
        // the hdfsRead api only return 65536Bytes data at one time
        do
        {
            readedBytes = hdfsRead(fs, m_currentFd->fs, buffer->data() + readedCount, dataLen);
            readedCount += readedBytes;
        } while (readedBytes != 0 && readedBytes != -1 && readedCount < dataLen);
        // read error
        if (readedBytes == -1)
        {
            BOOST_THROW_EXCEPTION(
                HDFSReadDataFailed() << bcos::errinfo_comment(
                    "loadDataAndCreateBufferParser: read-hdfs-data failed, error: " +
                    std::string(hdfsGetLastError())));
        }
        // the file has been read finished
        if (readedCount > 0)
        {
            IO_LOG(TRACE) << LOG_DESC("loadDataAndCreateBufferParser")
                          << LOG_KV("readedCount", readedCount);
            m_readedBytes += readedCount;
            buffer->resize(readedCount);
            // seek to the pointer
            hdfsSeek(fs, m_currentFd->fs, m_readedBytes);
            // allocate the currentBlockParser
            m_currentBlockParser = std::make_shared<BufferParser>(std::move(*buffer));
            return true;
        }
        return false;
    }

private:
    // the file handler
    ppc::storage::FileHandler::Ptr m_handler;
    // buffer size loaded per time
    uint64_t m_bufferSize;
    std::unique_ptr<HdfsFileWrapper, HDFSFileDeleter> m_currentFd;
    char*** m_hostsInfo;
    uint64_t m_readedBytes = 0;
    int m_currentBlock = 0;
    int m_currentBlockOffset = -1;
};
}  // namespace ppc::io