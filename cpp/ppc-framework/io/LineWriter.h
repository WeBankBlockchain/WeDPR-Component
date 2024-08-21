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
 * @file Writer.h
 * @author: yujiechen
 * @date 2022-10-17
 */
#pragma once
#include "../protocol/Protocol.h"
#include "DataBatch.h"
namespace ppc::io
{
class LineWriter
{
public:
    using Ptr = std::shared_ptr<LineWriter>;
    LineWriter() = default;
    virtual ~LineWriter() = default;

    virtual ppc::protocol::DataResourceType type() const = 0;

    // write the data into the writer(Note: only support row-data)
    virtual bool writeLine(
        DataBatch::ConstPtr _data, DataSchema _schema, std::string _lineSplitter = "\n") = 0;

    virtual void writeBytes(bcos::bytesConstRef _data) = 0;

    // flush the data into the storage-backend
    virtual void flush() = 0;
    // close the storage-backend resource(Note: flush before close)
    virtual void close() = 0;

    void setFileInfo(protocol::FileInfo::Ptr _fileInfo) { m_fileInfo = std::move(_fileInfo); }
    protocol::FileInfo::Ptr const& fileInfo() const { return m_fileInfo; }

    virtual void upload() {}

    virtual void clean() {}

private:
    protocol::FileInfo::Ptr m_fileInfo;
};
}  // namespace ppc::io