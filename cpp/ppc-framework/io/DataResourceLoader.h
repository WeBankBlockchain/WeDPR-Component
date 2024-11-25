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
 * @file DataResourceLoader.h
 * @author: yujiechen
 * @date 2022-11-4
 */

#pragma once
#include "../protocol/DataResource.h"
#include "../protocol/Protocol.h"
#include "../storage/FileStorage.h"
#include "LineReader.h"
#include "LineWriter.h"

namespace ppc::io
{
class DataResourceLoader
{
public:
    using Ptr = std::shared_ptr<DataResourceLoader>;
    DataResourceLoader() = default;
    virtual ~DataResourceLoader() = default;

    // load resource according to the dataResource
    // Note: the FILE type data resource only support parsed by line
    //       the FILE type data resource can be loaded by segment
    //       the SQL type data resource support parsed by both line and column
    //       all the data returned from the SQL will be loaded into the memory
    virtual LineReader::Ptr loadReader(ppc::protocol::DataResourceDesc::ConstPtr _dataResourceDesc,
        ppc::io::DataSchema _schema = ppc::io::DataSchema::String, bool _parseByColumn = true,
        ppc::storage::FileStorage::Ptr const& _storage = nullptr) = 0;

    // create a LineWriter to store data into the data-resource specified by outputDesc
    // Note: only support file/hdfs now
    virtual LineWriter::Ptr loadWriter(ppc::protocol::DataResourceDesc::ConstPtr _dataResourceDesc,
        bool _truncate = false, ppc::storage::FileStorage::Ptr const& _storage = nullptr) = 0;

    virtual void checkResourceExists(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::storage::FileStorage::Ptr const& _storage = nullptr) = 0;

    virtual void deleteResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        ppc::storage::FileStorage::Ptr const& _storage = nullptr) = 0;

    virtual void renameResource(ppc::protocol::DataResourceDesc::ConstPtr _desc,
        std::string const& _new_path, ppc::storage::FileStorage::Ptr const& _storage = nullptr) = 0;
};
}  // namespace ppc::io