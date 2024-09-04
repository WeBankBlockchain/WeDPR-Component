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
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include "ppc-rpc/src/RpcFactory.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <bcos-utilities/Common.h>
#include <string>

using namespace bcos;
using namespace ppc;
using namespace ppc::io;
using namespace ppc::rpc;
using namespace ppc::tools;

namespace ppc::cem
{
class CEMService
{
public:
    using Ptr = std::shared_ptr<CEMService>;
    CEMService() = default;
    virtual ~CEMService() = default;

    void makeCiphertextEqualityMatchRpc(Json::Value const& request, RespFunc func);
    void makeCiphertextEqualityMatch(Json::Value const& request, Json::Value& response, ppc::protocol::DataResourceType _type);
    void encryptDatasetRpc(Json::Value const& request, RespFunc func);
    void encryptDataset(Json::Value const& request, Json::Value& response);

    void setCEMConfig(CEMConfig const& cemConfig);
    void setStorageConfig(StorageConfig const& storageConfig);
    void doCipherTextEqualityMatch(const Json::Value::Members& fieldNames,
        const std::vector<std::string>& fieldValues, LineReader::Ptr lineReader,
        Json::Value& matchCount);
    void doEncryptDataset(LineReader::Ptr lineReader, LineWriter::Ptr lineWriter);
    LineReader::Ptr initialize_lineReader(const std::string& _datasetId, ppc::protocol::DataResourceType _type);
    LineWriter::Ptr initialize_lineWriter(const std::string& _datasetId, ppc::protocol::DataResourceType _type);
    void renameSource(const std::string& _datasetId, ppc::protocol::DataResourceType _type);

private:
    CEMConfig m_cemConfig;
    StorageConfig m_storageConfig;
};
}  // namespace ppc::cem