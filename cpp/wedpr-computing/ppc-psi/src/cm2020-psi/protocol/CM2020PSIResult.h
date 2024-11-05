/**
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
 * @file CM2020PSIResult.h
 * @author: shawnhe
 * @date 2022-12-13
 */
#pragma once

#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <json/json.h>
#include <memory>
#include <sstream>

#include "ppc-framework/protocol/Task.h"

namespace ppc::psi
{
// Note: can extend on demand
class CM2020PSIResult : public protocol::TaskResult
{
public:
    using Ptr = std::shared_ptr<CM2020PSIResult>;
    CM2020PSIResult(std::string const& _taskID) : TaskResult(_taskID) {}
    ~CM2020PSIResult() override = default;

    void setOutputs(std::vector<std::vector<std::string>>&& _outputs)
    {
        m_outputs = std::move(_outputs);
    }

    // serialize the taskResult to json
    [[nodiscard]] Json::Value serializeToJson() override
    {
        Json::Value response;
        response["taskID"] = taskID();
        if (m_timeCost)
        {
            response["timeCost"] = std::to_string(m_timeCost) + "ms";
        }
        if (m_fileInfo && !m_fileInfo->bizSeqNo.empty())
        {
            response["bizSeqNo"] = m_fileInfo->bizSeqNo;
            response["fileID"] = m_fileInfo->fileID;
            response["fileMd5"] = m_fileInfo->fileMd5;
        }

        if (m_error && error()->errorCode())
        {
            response["code"] = error()->errorCode();
            response["message"] = error()->errorMessage();
            response["status"] = protocol::toString(protocol::TaskStatus::FAILED);
        }
        else
        {
            response["code"] = 0;
            response["message"] = "success";
            response["status"] = protocol::toString(protocol::TaskStatus::COMPLETED);

            Json::Value jsonData;
            jsonData["bucketNumber"] = m_bucketNumber;
            jsonData["communication"] = m_communication;
            jsonData["syncResult"] = m_enableSyncResults;
            jsonData["intersections"] = m_intersections;
            //            jsonData["party0Size"] = m_party0Size;
            //            jsonData["party1Size"] = m_party1Size;

            if (!m_outputs.empty())
            {
                Json::Value jsonOutputs;
                for (auto& outputVector : m_outputs)
                {
                    Json::Value jsonOutput;
                    for (auto& output : outputVector)
                    {
                        jsonOutput.append(output);
                    }
                    jsonOutputs.append(jsonOutput);
                }
                jsonData["outputs"] = jsonOutputs;
            }

            response["data"] = jsonData;
        }
        return response;
    }


    std::vector<std::vector<std::string>> m_outputs;
    uint16_t m_bucketNumber;
    std::string m_communication;
    bool m_enableSyncResults;
    uint32_t m_intersections;
    uint32_t m_party0Size;
    uint32_t m_party1Size;
};

}  // namespace ppc::psi
