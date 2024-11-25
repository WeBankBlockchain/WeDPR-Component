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
 * @file TaskGuarder.h
 * @author: shawnhe
 * @date 2022-01-07
 */

#pragma once

#include "../Common.h"
#include "../PSIConfig.h"
#include "TaskState.h"
#include "ppc-framework/protocol/Protocol.h"
#include "protocol/src/PPCMessage.h"
#include <bcos-utilities/Timer.h>

namespace ppc::psi
{
class TaskGuarder
{
public:
    using Ptr = std::shared_ptr<TaskGuarder>;
    explicit TaskGuarder(PSIConfig::Ptr _config) : m_config(std::move(_config)) {}
    TaskGuarder(
        PSIConfig::Ptr _config, protocol::TaskAlgorithmType _type, const std::string& _timerName)
      : m_config(std::move(_config)),
        m_type(_type),
        m_taskStateFactory(std::make_shared<TaskStateFactory>())
    {
        m_pingTimer = std::make_shared<bcos::Timer>(c_pingTimerPeriod, _timerName);
    }
    virtual ~TaskGuarder() = default;

    virtual void onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const&){};
    virtual void onSelfError(
        const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer){};

    void startPingTimer()
    {
        m_pingTimer->registerTimeoutHandler([this]() { checkPeerActivity(); });
        m_pingTimer->start();
    }
    void stopPingTimer()
    {
        if (m_pingTimer)
        {
            m_pingTimer->stop();
        }
    }

    bcos::Error::Ptr checkTask(const ppc::protocol::Task::ConstPtr& _task, uint16_t _partiesCount,
        bool _enforceSelfInput, bool _enforceSelfOutput, bool _enforcePeerResource,
        bool _enforceSelfResource = true);

    // this only work for two-party task
    static std::string getPeerID(ppc::protocol::Task::ConstPtr _task)
    {
        auto const& peerParties = _task->getAllPeerParties();
        if (peerParties.empty())
        {
            return "";
        }
        return peerParties.begin()->second->id();
    }

    void noticePeerToFinish(const ppc::protocol::Task::ConstPtr& _task);
    void noticePeerToFinish(const std::string& _taskID, const std::string& _peer);

    void checkPeerActivity();

    io::LineReader::Ptr loadReader(std::string const& _taskID,
        protocol::DataResource::ConstPtr const& _dataResource, io::DataSchema _dataSchema,
        uint32_t _columnSize = 1);

    io::LineWriter::Ptr loadWriter(std::string const& _taskID,
        protocol::DataResource::ConstPtr const& _dataResource, bool _enableOutputExists);

    TaskState::Ptr findPendingTask(const std::string& _taskID);

    void addPendingTask(TaskState::Ptr _taskState);

    void removePendingTask(const std::string& _taskID);

protected:
    PSIConfig::Ptr m_config;
    protocol::TaskAlgorithmType m_type;
    TaskStateFactory::Ptr m_taskStateFactory;
    // the timer used to check the activity of peer node
    std::shared_ptr<bcos::Timer> m_pingTimer;

    const unsigned int c_pingTimerPeriod = 60000;

    std::unordered_map<std::string, TaskState::Ptr> m_pendingTasks;
    mutable bcos::SharedMutex x_pendingTasks;
};

}  // namespace ppc::psi