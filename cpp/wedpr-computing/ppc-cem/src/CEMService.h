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
 * @file CEMService.h
 * @author: caryliao
 * @date 2022-11-04
 */
#pragma once
#include "Common.h"
#include "ppc-framework/rpc/RpcInterface.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include "ppc-tools/src/config/CEMConfig.h"
#include "ppc-tools/src/config/StorageConfig.h"
#include <bcos-utilities/Common.h>
#include <string>
namespace ppc::cem
{
class CEMService
{
public:
    using Ptr = std::shared_ptr<CEMService>;
    CEMService() = default;
    virtual ~CEMService() = default;

    void makeCiphertextEqualityMatchRpc(Json::Value const& request, ppc::rpc::RespFunc func);
    void makeCiphertextEqualityMatch(
        Json::Value const& request, Json::Value& response, ppc::protocol::DataResourceType _type);
    void encryptDatasetRpc(Json::Value const& request, ppc::rpc::RespFunc func);
    void encryptDataset(Json::Value const& request, Json::Value& response);

    void setCEMConfig(ppc::tools::CEMConfig const& cemConfig);
    void setStorageConfig(ppc::tools::StorageConfig const& storageConfig);
    void doCipherTextEqualityMatch(const Json::Value::Members& fieldNames,
        const std::vector<std::string>& fieldValues, ppc::io::LineReader::Ptr lineReader,
        Json::Value& matchCount);
    void doEncryptDataset(ppc::io::LineReader::Ptr lineReader, ppc::io::LineWriter::Ptr lineWriter);
    ppc::io::LineReader::Ptr initialize_lineReader(
        const std::string& _datasetId, ppc::protocol::DataResourceType _type);
    ppc::io::LineWriter::Ptr initialize_lineWriter(
        const std::string& _datasetId, ppc::protocol::DataResourceType _type);
    void renameSource(const std::string& _datasetId, ppc::protocol::DataResourceType _type);

private:
    ppc::tools::CEMConfig m_cemConfig;
    ppc::tools::StorageConfig m_storageConfig;
};
}  // namespace ppc::cem