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
 * @file BaseFileLineReader.h
 * @author: yujiechen
 * @date 2022-10-19
 */
#pragma once
#include "parser/MmapParser.h"
#include "ppc-framework/io/LineReader.h"
#include <bcos-utilities/Common.h>

namespace ppc::io
{
class BaseFileLineReader : public LineReader
{
public:
    BaseFileLineReader(char _lineSpliter) : m_lineSplitter(_lineSpliter) {}
    ~BaseFileLineReader() override {}

    /**
     * @brief get the next _offset line data
     *
     * @param _size: the data-size to be read
     *               read all rows at once when the _size is -1
     * @return DataBatch::Ptr: the container that contains the readed-data
     */
    DataBatch::Ptr next(int64_t _size = -1, DataSchema schema = DataSchema::String) override;

    // the capacity of the file or memory-bytes
    uint64_t capacity() const override { return m_length; }

    uint64_t columnSize() const override { return 1; }
    bcos::bytes readBytes() override;

protected:
    virtual bool allocateCurrentBlock() = 0;


    template <typename T>
    int64_t readLineFromBlock(DataBatch::Ptr _dataBatch, int64_t _size)
    {
        int64_t readedLineNum = 0;
        int64_t readedBytes = 0;
        for (; readedLineNum < _size; readedLineNum++)
        {
            T dataItem;
            auto readLineResult =
                m_currentBlockParser->readLine(readedBytes, dataItem, m_lineSplitter);
            // update the offset
            m_offset += readedBytes;
            if (m_appendToLine)
            {
                m_appendToLine = false;
                _dataBatch->appendToLine(std::move(dataItem));
            }
            else
            {
                _dataBatch->append(std::move(dataItem));
            }
            if (ReadLineResult::ReadFinishWithoutSpliter == readLineResult)
            {
                m_appendToLine = true;
            }
            // reset the current mmap-file since it has been read-finished
            if (ReadLineResult::ReadFinish == readLineResult ||
                ReadLineResult::ReadFinishWithoutSpliter == readLineResult)
            {
                m_currentBlockParser.reset();
                break;
            }
        }
        IO_LOG(INFO) << LOG_DESC("readLineFromBlock") << LOG_KV("readedLineNum", readedLineNum)
                     << LOG_KV("expectedSize", _size) << LOG_KV("offset", m_offset);
        return readedLineNum;
    }

    template <typename T>
    DataBatch::Ptr readLineData(int64_t _size, DataSchema _schema)
    {
        auto dataBatch = std::make_shared<DataBatch>();
        dataBatch->setDataSchema(_schema);
        int64_t expectedSize = _size;
        while (expectedSize > 0 && (allocateCurrentBlock()))
        {
            auto readLineNum = readLineFromBlock<T>(dataBatch, expectedSize);
            expectedSize = expectedSize - readLineNum;
        }
        IO_LOG(INFO) << LOG_DESC("readLineData") << LOG_KV("size", _size)
                     << LOG_KV("readedSize", dataBatch->size());
        return dataBatch;
    }

    template <typename T>
    DataBatch::Ptr readAllData(DataSchema _dataSchema)
    {
        auto dataBatch = std::make_shared<DataBatch>();
        dataBatch->setDataSchema(_dataSchema);
        while (allocateCurrentBlock())
        {
            readLineFromBlock<T>(dataBatch, m_readGranularity);
        }
        IO_LOG(INFO) << LOG_DESC("readAllData") << LOG_KV("size", dataBatch->size());
        return dataBatch;
    }

protected:
    char m_lineSplitter;
    uint64_t m_length;
    BaseBufferParser::Ptr m_currentBlockParser;  // the current block-parser

    mutable uint64_t m_offset = 0;  // the current read pointer
    bool m_appendToLine = false;

    // Note: the m_readGranularity should be set by the override class
    uint64_t m_readGranularity;
};
}  // namespace ppc::io