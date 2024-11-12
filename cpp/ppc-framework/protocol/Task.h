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
 * @file PartyResource.h
 * @author: yujiechen
 * @date 2022-10-13
 */
#pragma once
#include "PartyResource.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <json/json.h>
#include <memory>
#include <sstream>

namespace ppc::protocol
{
struct GatewayTaskInfo
{
    using Ptr = std::shared_ptr<GatewayTaskInfo>;
    GatewayTaskInfo(std::string const& _taskID) : taskID(_taskID) {}
    GatewayTaskInfo() = default;

    std::string taskID;
    // Note: optional for the app-level
    std::string serviceEndpoint;
};


// Note: can extend on demand
class TaskResult
{
public:
    using Ptr = std::shared_ptr<TaskResult>;
    TaskResult(std::string const& _taskID) : m_taskID(_taskID) {}
    virtual ~TaskResult() = default;

    virtual std::string const& taskID() const { return m_taskID; }

    virtual std::string const& status() const { return m_status; }
    virtual void setStatus(const std::string& _status) { m_status = _status; }

    virtual bcos::Error::Ptr error() const { return m_error; }
    virtual void setError(bcos::Error::Ptr _error) { m_error = std::move(_error); }

    virtual uint64_t timeCost() const { return m_timeCost; }
    virtual void setTimeCost(uint64_t _cost) { m_timeCost = _cost; }

    virtual FileInfo::Ptr fileInfo() const { return m_fileInfo; }
    virtual void setFileInfo(FileInfo::Ptr _fileInfo) { m_fileInfo = std::move(_fileInfo); }

    // serialize the taskResult to json
    virtual Json::Value serializeToJson()
    {
        Json::Value response;
        response["taskID"] = m_taskID;
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
        if (m_error && m_error->errorCode() != 0)
        {
            response["code"] = m_error->errorCode();
            response["message"] = m_error->errorMessage();
        }
        else
        {
            response["code"] = 0;
            response["message"] = "success";
        }
        if (!m_status.empty())
        {
            response["status"] = m_status;
        }
        return response;
    }

protected:
    std::string m_taskID;
    std::string m_status;
    uint64_t m_timeCost = 0;
    bcos::Error::Ptr m_error = nullptr;
    FileInfo::Ptr m_fileInfo = nullptr;
};

class Task
{
public:
    using Ptr = std::shared_ptr<Task>;
    using ConstPtr = std::shared_ptr<Task const>;
    Task() = default;
    virtual ~Task() = default;

    // the taskID
    virtual std::string const& id() const = 0;
    // the taskType, e.g. PSI
    virtual uint8_t type() const = 0;
    // the algorithm
    virtual uint8_t algorithm() const = 0;
    // information of the local party
    virtual PartyResource::ConstPtr selfParty() const = 0;
    virtual PartyResource::Ptr mutableSelfParty() const = 0;
    // information of the peer party
    virtual PartyResource::ConstPtr getPartyByID(std::string const& _partyID) const = 0;
    virtual PartyResource::ConstPtr getPartyByIndex(uint16_t _partyIndex) const = 0;
    // get all peer-parties
    virtual std::map<std::string, PartyResource::Ptr> const& getAllPeerParties() const = 0;
    // params of the task, can be deserialized using json
    virtual std::string const& param() const = 0;
    // sync the psi result to peer or not
    virtual bool syncResultToPeer() const = 0;
    virtual bool lowBandwidth() const = 0;
    virtual std::vector<std::string> const& getReceiverLists() const = 0;

    // the setters
    virtual void setId(std::string const& _id) = 0;
    virtual void setType(uint8_t _type) = 0;
    virtual void setAlgorithm(uint8_t _algorithm) = 0;
    virtual void setSelf(PartyResource::Ptr _self) = 0;
    virtual void addParty(PartyResource::Ptr _party) = 0;
    virtual void setParam(std::string const& _param) = 0;
    virtual void setSyncResultToPeer(bool _syncResultToPeer) = 0;
    virtual void setLowBandwidth(bool _lowBandwidth) = 0;
    // decode the task
    virtual void decode(std::string_view _taskData) = 0;
    virtual std::string encode() const = 0;
};

class TaskFactory
{
public:
    using Ptr = std::shared_ptr<TaskFactory>;
    using ConstPtr = std::shared_ptr<TaskFactory const>;
    TaskFactory() = default;
    virtual ~TaskFactory() = default;

    virtual Task::Ptr createTask(std::string_view _taskInfo) = 0;
    virtual Task::Ptr createTask() = 0;
};

inline std::string printTaskInfo(Task::ConstPtr _task)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("id", _task->id())
                 << LOG_KV("type", (ppc::protocol::TaskType)_task->type())
                 << LOG_KV("algorithm", (ppc::protocol::TaskAlgorithmType)_task->algorithm())
                 << LOG_KV("taskPtr", _task);
    if (_task->selfParty())
    {
        stringstream << printPartyInfo(_task->selfParty());
    }
    return stringstream.str();
}
}  // namespace ppc::protocol
