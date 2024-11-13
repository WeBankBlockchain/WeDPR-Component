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
 * @file FileLineReader.h
 * @author: yujiechen
 * @date 2022-10-19
 */
#pragma once
#include "BaseFileLineReader.h"
#include "parser/MmapParser.h"
#include "ppc-framework/io/LineReader.h"
#include <bcos-utilities/Common.h>

namespace ppc::io
{
class FileLineReader : public BaseFileLineReader
{
public:
    using Ptr = std::shared_ptr<FileLineReader>;
    // Note: the same file can't be read/write in the same time
    FileLineReader(
        std::string const& _filePath, int64_t _mmapGranularity = -1, char _lineSpliter = '\n');
    ~FileLineReader() override;

    ppc::protocol::DataResourceType type() const override
    {
        return ppc::protocol::DataResourceType::FILE;
    }
    bool readFinished() const override { return (m_offset == m_length); }

protected:
    bool allocateCurrentBlock() override;

protected:
    std::string m_filePath;
    int c_pageSize;
    // Note: the mmapGranularity must a multiple of page-size
    uint64_t m_mmapGranularity;  // the mmap-granularity, default is (500 * 1024 * pageSize)

    int m_fd = -1;
};

class FileLineReaderFactory : public LineReaderFactory
{
public:
    using Ptr = std::shared_ptr<FileLineReaderFactory>;
    FileLineReaderFactory() = default;
    ~FileLineReaderFactory() override = default;

    LineReader::Ptr createLineReader(std::string const& _filePath,
        uint64_t _mmapGranularity = 500 * 1024 * 1024, char _lineSpliter = '\n') override
    {
        return std::make_shared<FileLineReader>(_filePath, _mmapGranularity, _lineSpliter);
    }
};
}  // namespace ppc::io