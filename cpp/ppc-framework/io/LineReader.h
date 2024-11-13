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
 * @file Reader.h
 * @author: yujiechen
 * @date 2022-10-13
 */
#pragma once
#include "../protocol/Protocol.h"
#include "DataBatch.h"

namespace ppc::io
{
class LineReader
{
public:
    using Ptr = std::shared_ptr<LineReader>;
    LineReader() = default;
    virtual ~LineReader() = default;

    // get the next _offset line data
    virtual DataBatch::Ptr next(int64_t _size, DataSchema schema = DataSchema::String) = 0;
    virtual bcos::bytes readBytes() = 0;
    // the capacity of the file or memory-bytes
    virtual uint64_t capacity() const = 0;
    virtual uint64_t columnSize() const = 0;
    virtual ppc::protocol::DataResourceType type() const = 0;
    virtual void clean() {}
    virtual bool readFinished() const = 0;
};

class LineReaderFactory
{
public:
    using Ptr = std::shared_ptr<LineReaderFactory>;
    LineReaderFactory() = default;
    virtual ~LineReaderFactory() = default;

    virtual LineReader::Ptr createLineReader(std::string const& _filePath,
        uint64_t _mmapGranularity = 500 * 1024 * 1024, char _lineSpliter = '\n') = 0;
};
}  // namespace ppc::io