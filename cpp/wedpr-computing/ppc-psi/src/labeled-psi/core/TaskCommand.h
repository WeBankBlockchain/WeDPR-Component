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
 * @file TaskCommand.h
 * @author: shawnhe
 * @date 2022-11-14
 */

#pragma once
#include "../Common.h"
#include <bcos-utilities/Error.h>
#include <json/json.h>

namespace ppc::psi
{
class TaskCommand
{
public:
    using Ptr = std::shared_ptr<TaskCommand>;
    TaskCommand(std::string_view _param)
    {
        Json::Reader reader;
        Json::Value result;
        if (!reader.parse(_param.begin(), _param.end(), result))
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int)LabeledPSIRetCode::INVALID_TASK_PARAM, "invalid task param: invalid json"));
        }
        if (!result.isArray() || result.empty())
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)LabeledPSIRetCode::INVALID_TASK_PARAM,
                "invalid task param:: the param must be array"));
        }
        auto command = result[0].asString();
        if (command == CMD_SETUP_SENDER_DB)
        {
            m_command = (int)LabeledPSICommand::SETUP_SENDER_DB;
            if (result.size() < 2)
            {
                BOOST_THROW_EXCEPTION(BCOS_ERROR((int)LabeledPSIRetCode::INVALID_TASK_PARAM,
                    "must set label_byte_count for command: " + command));
            }
            m_args.emplace_back(std::stoi(result[1].asString()));
        }
        else if (command == CMD_RUN_LABELED_PSI)
        {
            m_command = (int)LabeledPSICommand::RUN_LABELED_PSI;
        }
        else if (command == CMD_SAVE_SENDER_CACHE)
        {
            m_command = (int)LabeledPSICommand::SAVE_SENDER_CACHE;
        }
        else if (command == CMD_LOAD_SENDER_CACHE)
        {
            m_command = (int)LabeledPSICommand::LOAD_SENDER_CACHE;
        }
        else if (command == CMD_ADD_ITEMS)
        {
            m_command = (int)LabeledPSICommand::ADD_ITEMS;
        }
        else if (command == CMD_DELETE_ITEMS)
        {
            m_command = (int)LabeledPSICommand::DELETE_ITEMS;
        }
        else if (command == CMD_UPDATE_LABELS)
        {
            m_command = (int)LabeledPSICommand::UPDATE_LABELS;
        }
        else
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int)LabeledPSIRetCode::UNDEFINED_COMMAND, "unsupported command: " + command));
        }
    }

    int command() const { return m_command; }
    std::vector<int> const& args() const { return m_args; }

private:
    int m_command;
    std::vector<int> m_args;
};
}  // namespace ppc::psi
