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
 * @file LabeledPSIResult.h
 * @author: shawnhe
 * @date 2022-11-13
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
class LabeledPSIResult : public protocol::TaskResult
{
public:
    using Ptr = std::shared_ptr<LabeledPSIResult>;
    LabeledPSIResult(std::string const& _taskID) : TaskResult(_taskID) {}
    virtual ~LabeledPSIResult() = default;


    void setOutputs(std::vector<std::vector<std::string>>&& _outputs)
    {
        m_outputs = std::move(_outputs);
    }

    const std::vector<std::vector<std::string>>& getOutputs() { return m_outputs; }

    // serialize the taskResult to json
    Json::Value serializeToJson() override
    {
        Json::Value response;
        response["taskID"] = taskID();
        if (m_timeCost)
        {
            response["timeCost"] = std::to_string(m_timeCost) + "ms";
        }
        if (error() && error()->errorCode())
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
        }
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
            Json::Value jsonData;
            jsonData["outputs"] = jsonOutputs;
            response["data"] = jsonData;
        }
        return response;
    }

private:
    std::vector<std::vector<std::string>> m_outputs;
};

}  // namespace ppc::psi
