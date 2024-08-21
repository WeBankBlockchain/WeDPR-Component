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
#pragma once
#include "Common.h"
#include "ppc-framework/io/LineWriter.h"
#include "ppc-framework/storage/FileStorage.h"

namespace ppc::io
{
class HDFSWriter : public LineWriter
{
public:
    using Ptr = std::shared_ptr<HDFSWriter>;
    HDFSWriter(ppc::storage::FileHandler::Ptr _handler, bool _trunc,
        int64_t _cacheSize = 100 * 1024 * 1024);
    ~HDFSWriter() override { close(); }

    ppc::protocol::DataResourceType type() const override
    {
        return ppc::protocol::DataResourceType::HDFS;
    }

    // write the data into the writer(Note: only support row-data)
    bool writeLine(
        DataBatch::ConstPtr _data, DataSchema _schema, std::string _lineSplitter = "\n") override;

    void writeBytes(bcos::bytesConstRef _data) override;

    // flush the data into the storage-backend
    void flush() override;
    // close the storage-backend resource(Note: flush before close)
    void close() override;

protected:
    void flushCache(bool _enforce)
    {
        if (m_cache.size() == 0)
        {
            return;
        }
        if (!_enforce && m_cache.size() < m_cacheSize)
        {
            return;
        }
        // flush cache into the hdfs, clear the cache
        auto ret = hdfsWrite(
            (hdfsFS)m_handler->storageHandler(), m_fd->fs, m_cache.data(), m_cache.size());
        if (ret == -1)
        {
            BOOST_THROW_EXCEPTION(
                HDFSWriteDataFailed() << bcos::errinfo_comment(
                    "write cache-data failed, error: " + std::string(hdfsGetLastError())));
        }
        m_cache.clear();
    }

    template <typename T>
    void writeData(DataBatch::ConstPtr _data, std::string _lineSplitter)
    {
        for (uint64_t i = 0; i < _data->size(); i++)
        {
            if (m_cache.size() >= m_cacheSize)
            {
                flushCache(false);
            }
            auto& lineData = _data->get<T>(i);
            m_cache.insert(m_cache.end(), reinterpret_cast<const char*>(lineData.data()),
                reinterpret_cast<const char*>(lineData.data() + lineData.size()));
            if (_lineSplitter.empty())
            {
                continue;
            }
            m_cache.insert(m_cache.end(), reinterpret_cast<const char*>(_lineSplitter.data()),
                reinterpret_cast<const char*>(_lineSplitter.data() + _lineSplitter.size()));
        }
        flushCache(true);
    }

private:
    ppc::storage::FileHandler::Ptr m_handler;
    std::unique_ptr<HdfsFileWrapper, HDFSFileDeleter> m_fd;

    int64_t m_cacheSize;
    bcos::bytes m_cache;
};
}  // namespace ppc::io