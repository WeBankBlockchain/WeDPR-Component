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
 * @file Ra2018TaskParam.h
 * @author: yujiechen
 * @date 2022-11-7
 */
#pragma once
#include "../Common.h"
#include <json/json.h>
namespace ppc::psi
{
class Ra2018TaskParam
{
public:
    using Ptr = std::shared_ptr<Ra2018TaskParam>;
    Ra2018TaskParam(std::string_view _param)
    {
        Json::Reader reader;
        Json::Value result;
        if (!reader.parse(_param.begin(), _param.end(), result))
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::InvalidTaskParamForRA2018,
                "Invalid Ra2018TaskParam: invalid json"));
        }
        if (!result.isArray())
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::InvalidTaskParamForRA2018,
                "Invalid Ra2018TaskParam: the param must be array"));
        }
        auto command = result[0].asString();
        // the DATA_PREPROCESSING command must specified the operationType(insert(0)/delete(1))
        if (DATA_PREPROCESSING_CMD == command)
        {
            m_command = RA2018Command::DATA_PREPROCESSING;
            if (result.size() < 2)
            {
                BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::InvalidTaskParamForRA2018,
                    "Must set operation for " + command + ", insert(0)/delete(1)"));
            }
            m_operation = result[1].asInt();
        }
        else if (RUN_PSI_CMD == command)
        {
            m_command = RA2018Command::RUN_PSI;
        }
        else
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int)PSIRetCode::InvalidTaskParamForRA2018, "Unsupported command " + command));
        }
    }

    int command() const { return m_command; }
    int operation() const { return m_operation; }

private:
    int m_command;
    int m_operation;
};
}  // namespace ppc::psi
