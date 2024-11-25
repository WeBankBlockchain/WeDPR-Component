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
 * @file DataResource.h
 * @author: yujiechen
 * @date 2022-10-13
 */
#pragma once
#include "Protocol.h"
#include <memory>
#include <sstream>
#include <string>

namespace ppc::protocol
{
class DataResourceDesc
{
public:
    using Ptr = std::shared_ptr<DataResourceDesc>;
    using ConstPtr = std::shared_ptr<const DataResourceDesc>;
    DataResourceDesc() = default;
    virtual ~DataResourceDesc() = default;

    virtual uint16_t type() const { return m_type; }
    virtual void setType(uint16_t _type) { m_type = _type; }

    virtual std::string const& path() const { return m_path; }
    virtual void setPath(std::string const& _path) { m_path = _path; }

    virtual std::string const& accessCommand() const { return m_accessCommand; }
    virtual void setAccessCommand(std::string const& _command) { m_accessCommand = _command; }

    virtual std::string const& fileID() const { return m_fileID; }
    virtual void setFileID(std::string const& _fileID) { m_fileID = _fileID; }

    virtual std::string const& fileMd5() const { return m_fileMd5; }
    virtual void setFileMd5(std::string const& _fileMd5) { m_fileMd5 = _fileMd5; }

    virtual std::string const& bizSeqNo() const { return m_bizSeqNo; }
    virtual void setBizSeqNo(std::string const& _bizSeqNo) { m_bizSeqNo = _bizSeqNo; }

    virtual SQLConnectionOption::Ptr const& sqlConnectionOption() const
    {
        return m_sqlConnectionOption;
    }
    virtual void setSQLConnectionOption(SQLConnectionOption::Ptr const& _option)
    {
        m_sqlConnectionOption = _option;
    }

    // the hdfs connection
    ppc::protocol::FileStorageConnectionOption::Ptr const& fileStorageConnectionOption() const
    {
        return m_fileStorageConnectionOption;
    }
    virtual void setFileStorageConnectOption(
        ppc::protocol::FileStorageConnectionOption::Ptr const& _fileStorageConnectionOption)
    {
        m_fileStorageConnectionOption = _fileStorageConnectionOption;
    }


private:
    std::string m_path;
    std::string m_accessCommand;
    std::string m_fileID;
    std::string m_fileMd5;
    std::string m_bizSeqNo;

    // the sql-connection-option
    SQLConnectionOption::Ptr m_sqlConnectionOption;
    // the file-storage-connection-option
    FileStorageConnectionOption::Ptr m_fileStorageConnectionOption;

    uint16_t m_type;
};

class DataResource
{
public:
    using Ptr = std::shared_ptr<DataResource>;
    using ConstPtr = std::shared_ptr<DataResource const>;
    using OriginData = std::vector<std::vector<std::string>>;
    DataResource() = default;
    DataResource(std::string const& _resourceID) : m_resourceID(_resourceID) {}

    virtual ~DataResource() = default;
    virtual std::string const& resourceID() const { return m_resourceID; }

    virtual DataResourceDesc::ConstPtr desc() const { return m_desc; }
    virtual DataResourceDesc::Ptr mutableDesc() const { return m_desc; }
    virtual DataResourceDesc::ConstPtr outputDesc() const { return m_outputDesc; }
    virtual DataResourceDesc::Ptr mutableOutputDesc() const { return m_outputDesc; }
    virtual OriginData const& rawData() const { return m_rawData; }

    virtual void setResourceID(std::string const& _id) { m_resourceID = _id; }
    virtual void setDesc(DataResourceDesc::Ptr _desc) { m_desc = _desc; }
    virtual void setOutputDesc(DataResourceDesc::Ptr _desc) { m_outputDesc = _desc; }
    virtual void setRawData(OriginData const& _data) { m_rawData = _data; }

private:
    std::string m_resourceID;
    DataResourceDesc::Ptr m_desc;
    DataResourceDesc::Ptr m_outputDesc;
    OriginData m_rawData;
};

inline std::string printDataResourceInfo(DataResource::ConstPtr _dataResource)
{
    if (!_dataResource)
    {
        return "empty";
    }
    std::ostringstream stringstream;
    stringstream << LOG_KV("dataResource", _dataResource->resourceID());
    if (_dataResource->desc())
    {
        stringstream << LOG_KV("dataType", _dataResource->desc()->type())
                     << LOG_KV("command", _dataResource->desc()->accessCommand())
                     << LOG_KV("path", _dataResource->desc()->path());
    }
    if (_dataResource->outputDesc())
    {
        stringstream << LOG_KV("outputType", _dataResource->outputDesc()->type())
                     << LOG_KV("outputPath", _dataResource->outputDesc()->path());
    }
    return stringstream.str();
}
}  // namespace ppc::protocol