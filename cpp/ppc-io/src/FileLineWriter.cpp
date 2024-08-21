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
 * @file FileLineWriter.cpp
 * @author: yujiechen
 * @date 2022-10-21
 */
#include "FileLineWriter.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace ppc::io;
using namespace bcos;
FileLineWriter::FileLineWriter(std::string const& _filePath, bool _trunc) : m_filePath(_filePath)
{
    boost::filesystem::path filePath(m_filePath);
    auto parentPath = filePath.parent_path();
    // the parentPath is a file
    if (!parentPath.empty() && boost::filesystem::exists(filePath.parent_path()) &&
        !boost::filesystem::is_directory(parentPath))
    {
        BOOST_THROW_EXCEPTION(
            OpenFileLineWriterException() << bcos::errinfo_comment(
                "open " + m_filePath + " error for the non-dir-parent-path exists!"));
    }
    try
    {
        // the parentPath not exists, create the parentPath
        if (!parentPath.empty() && !boost::filesystem::is_directory(parentPath))
        {
            boost::filesystem::create_directories(parentPath);
        }
        m_outStream.open(m_filePath, std::ios::binary | (_trunc ? std::ios::trunc : std::ios::app));
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(
            OpenFileLineWriterException() << bcos::errinfo_comment(
                "open " + m_filePath + " error: " + boost::diagnostic_information(e)));
    }
}

// write the data into the writer(Note: only support row-data)
bool FileLineWriter::writeLine(
    DataBatch::ConstPtr _data, DataSchema _schema, std::string _lineSplitter)
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
}

void FileLineWriter::writeBytes(bytesConstRef _data)
{
    m_outStream.write(reinterpret_cast<const char*>(_data.data()), _data.size());
}


// flush the data into the storage-backend
void FileLineWriter::flush()
{
    m_outStream.flush();
}
// close the storage-backend resource(Note: flush before close)
void FileLineWriter::close()
{
    m_outStream.close();
}