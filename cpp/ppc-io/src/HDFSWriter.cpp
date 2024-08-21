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
 * @file HDFSWriter.h
 * @author: yujiechen
 * @date 2022-12-1
 */
#include "HDFSWriter.h"

using namespace ppc::io;
using namespace ppc::storage;
using namespace bcos;

HDFSWriter::HDFSWriter(FileHandler::Ptr _handler, bool _trunc, int64_t _cacheSize)
  : m_handler(std::move(_handler)), m_cacheSize(_cacheSize)
{
    IO_LOG(INFO) << LOG_DESC("HDFSWriter") << LOG_KV("path", m_handler->path());
    int flag = O_WRONLY;
    if (!_trunc)
    {
        flag |= O_APPEND;
    }
    auto rawFd =
        hdfsOpenFile((hdfsFS)m_handler->storageHandler(), m_handler->path().c_str(), flag, 0, 0, 0);
    if (!rawFd)
    {
        BOOST_THROW_EXCEPTION(OpenFileFailed() << errinfo_comment(
                                  "HDFSWriter: open file " + m_handler->path() + " failed!"));
    }

    m_fd.reset(new HdfsFileWrapper(m_handler, rawFd));
}
// write the data into the writer(Note: only support row-data)
bool HDFSWriter::writeLine(DataBatch::ConstPtr _data, DataSchema _schema, std::string _lineSplitter)
{
    switch (_schema)
    {
    case DataSchema::String:
    {
        writeData<std::string>(_data, _lineSplitter);
        return true;
    }
    case DataSchema::Bytes:
    {
        writeData<bcos::bytes>(_data, _lineSplitter);
        return true;
    }
    default:
    {
        BOOST_THROW_EXCEPTION(UnsupportedDataSchema() << bcos::errinfo_comment(
                                  "Only support String and Bytes schema now!"));
    }
    }
    return false;
}

void HDFSWriter::writeBytes(bcos::bytesConstRef _data)
{
    auto ret = hdfsWrite((hdfsFS)m_handler->storageHandler(), m_fd->fs, _data.data(), _data.size());
    if (ret == -1)
    {
        BOOST_THROW_EXCEPTION(HDFSWriteDataFailed() << bcos::errinfo_comment(
                                  "writeBytes failed, error: " + std::string(hdfsGetLastError())));
    }
}

// flush the data into the storage-backend
// Note: we should not calls flush multiple times
void HDFSWriter::flush()
{
    if (hdfsFlush((hdfsFS)m_handler->storageHandler(), m_fd->fs) == -1)
    {
        BOOST_THROW_EXCEPTION(HDFSFlushFailed() << bcos::errinfo_comment(
                                  "flush failed, error: " + std::string(hdfsGetLastError())));
    }
}
// close the storage-backend resource(Note: flush before close)
void HDFSWriter::close()
{
    m_fd.reset();
}