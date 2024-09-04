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
 * @file FileLineWriter.h
 * @author: yujiechen
 * @date 2022-10-21
 */
#pragma once
#include "Common.h"
#include "ppc-framework/io/LineWriter.h"
#include <fstream>

namespace ppc::io
{
class FileLineWriter : public LineWriter
{
public:
    using Ptr = std::shared_ptr<FileLineWriter>;
    FileLineWriter(std::string const& _filePath, bool _trunc = false);
    ~FileLineWriter() override
    {
        try
        {
            flush();
            close();
        }
        catch (std::exception const& e)
        {
            BOOST_THROW_EXCEPTION(
                CloseFileLineWriterException() << bcos::errinfo_comment(
                    "close " + m_filePath + " error: " + boost::diagnostic_information(e)));
        }
    }

    ppc::protocol::DataResourceType type() const override
    {
        return ppc::protocol::DataResourceType::FILE;
    }

    // write the data into the writer(Note: only support row-data)
    bool writeLine(
        DataBatch::ConstPtr _data, DataSchema _schema, std::string _lineSplitter) override;

    void writeBytes(bcos::bytesConstRef _data) override;

    // flush the data into the storage-backend
    void flush() override;
    // close the storage-backend resource(Note: flush before close)
    void close() override;

private:
    template <typename T>
    void writeData(DataBatch::ConstPtr _dataBatch, std::string _lineSplitter)
    {
        IO_LOG(TRACE) << LOG_DESC("writeData, size:") << _dataBatch->size();
        for (uint64_t i = 0; i < _dataBatch->size(); i++)
        {
            auto const& element = _dataBatch->get<T>(i);
            m_outStream.write(reinterpret_cast<const char*>(element.data()), element.size());
            if (_lineSplitter.empty())
            {
                continue;
            }
            m_outStream.write(_lineSplitter.c_str(), _lineSplitter.size());
        }
    }

protected:
    std::ofstream m_outStream;
    std::string m_filePath;
};
}  // namespace ppc::io