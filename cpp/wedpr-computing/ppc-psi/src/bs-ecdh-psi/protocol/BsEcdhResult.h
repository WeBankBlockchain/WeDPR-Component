/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file BsModeResult.h
 * @author: shawnhe
 * @date 2023-09-21
 */
#pragma once

#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <json/json.h>
#include <memory>
#include <sstream>
#include <utility>

#include "ppc-framework/protocol/Task.h"

namespace ppc::psi
{
class BsEcdhResult : public protocol::TaskResult
{
public:
    using Ptr = std::shared_ptr<BsEcdhResult>;
    BsEcdhResult(std::string const& _taskID, Json::Value _data = Json::Value{})
      : TaskResult(_taskID), m_data(std::move(_data))
    {}
    ~BsEcdhResult() override = default;

    [[nodiscard]] Json::Value const& data() const { return m_data; }

    // serialize the taskResult to json
    [[nodiscard]] Json::Value serializeToJson() override
    {
        Json::Value response;
        if (m_error && error()->errorCode())
        {
            response["code"] = error()->errorCode();
            response["message"] = error()->errorMessage();
        }
        else
        {
            response["code"] = 0;
            response["message"] = "success";
        }
        if (!m_data.empty())
        {
            response["data"] = m_data;
        }
        return response;
    }

private:
    Json::Value m_data;
};

}  // namespace ppc::psi
