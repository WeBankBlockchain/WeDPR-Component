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
 * @file BsEcdhTaskState.h
 * @author: shawnhe
 * @date 2023-09-22
 */

#pragma once

#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/io/LineReader.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-psi/src/bs-ecdh-psi/Common.h"
#include "ppc-psi/src/bs-ecdh-psi/protocol/BsEcdhResult.h"
#include "ppc-psi/src/psi-framework/TaskGuarder.h"
#include <atomic>
#include <cstdint>
#include <utility>

namespace ppc::psi
{
#define BS_VALIDITY_TERM 86400000
#define MIN_BS_ACTIVE_COUNT 3

class BsEcdhTaskState
{
public:
    using Ptr = std::shared_ptr<BsEcdhTaskState>;

    BsEcdhTaskState(std::string _taskID, protocol::TaskStatus _status, uint32_t _timeoutMinutes)
      : m_taskID(std::move(_taskID)), m_status(_status), m_timeoutMinutes(_timeoutMinutes)
    {
        m_latestActiveTime = bcos::utcSteadyTime();
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("new BsEcdhTaskState") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("createTime", m_latestActiveTime)
                              << LOG_KV("timeoutMinutes", m_timeoutMinutes);
    }

    virtual ~BsEcdhTaskState() = default;

    [[nodiscard]] std::string taskID() const { return m_taskID; }

    [[nodiscard]] protocol::TaskStatus status() const
    {
        bcos::ReadGuard l(x_state);
        return m_status;
    }
    void updateStatus(protocol::TaskStatus _status)
    {
        bcos::WriteGuard l(x_state);
        m_status = _status;
    }

    [[nodiscard]] BsEcdhResult::Ptr result() const
    {
        bcos::ReadGuard l(x_state);
        return m_result;
    }
    void setResult(BsEcdhResult::Ptr _result)
    {
        bcos::WriteGuard l(x_state);
        m_result = std::move(_result);
    }

    void autoPauseChecking()
    {
        bcos::WriteGuard l(x_state);
        // trigger automatic pause
        if (m_status == protocol::TaskStatus::RUNNING &&
            m_latestActiveTime + m_autoPauseThreshold <= bcos::utcSteadyTime())
        {
            turnToPausing();
            BS_ECDH_PSI_LOG(INFO) << LOG_DESC("task is pausing") << LOG_KV("taskID", m_taskID);
        }
    }

    [[nodiscard]] bool isTimeout()
    {
        bcos::WriteGuard l(x_state);
        auto timeout =
            isExecutable(m_status) &&
            m_latestActiveTime + uint64_t(m_timeoutMinutes * 60 * 1000) <= bcos::utcSteadyTime();
        if (timeout)
        {
            BS_ECDH_PSI_LOG(WARNING) << LOG_DESC("task is timeout") << LOG_KV("taskID", m_taskID);
            GetTaskStatusResponse response;
            response.taskID = m_taskID;
            response.status = toString(protocol::TaskStatus::FAILED);
            m_result = std::make_shared<BsEcdhResult>(m_taskID, response.serialize());
            m_result->setError(
                std::make_shared<bcos::Error>((int)PSIRetCode::TaskTimeout, "Task is timeout."));
            m_status = protocol::TaskStatus::FAILED;
        }
        return timeout;
    }

    [[nodiscard]] bool isExpired() const
    {
        bcos::ReadGuard l(x_state);
        return m_latestActiveTime + BS_VALIDITY_TERM <= bcos::utcSteadyTime();
    }

    void active()
    {
        bcos::WriteGuard l(x_state);

        // The task is activated
        if (m_status == protocol::TaskStatus::PAUSING && ++activeCount == MIN_BS_ACTIVE_COUNT)
        {
            activeCount = 0;
            turnToRunning();
        }

        m_latestActiveTime = bcos::utcSteadyTime();
    }

    void setupAutoPause()
    {
        bcos::WriteGuard l(x_state);
        turnToRunning();
    }

    void cancelAutoPause()
    {
        bcos::WriteGuard l(x_state);
        m_autoPauseThreshold = BS_VALIDITY_TERM;
    }

    void pauseTask()
    {
        bcos::WriteGuard l(x_state);
        if (m_status == protocol::TaskStatus::RUNNING)
        {
            turnToPausing();
        }
    }

    void restartTask()
    {
        bcos::WriteGuard l(x_state);
        if (m_status == protocol::TaskStatus::PAUSING)
        {
            turnToRunning();
        }
    }

private:
    void turnToRunning()
    {
        m_status = protocol::TaskStatus::RUNNING;
        m_autoPauseThreshold = PAUSE_THRESHOLD;
        m_latestActiveTime = bcos::utcSteadyTime();
    }

    void turnToPausing()
    {
        m_status = protocol::TaskStatus::PAUSING;
        m_autoPauseThreshold = BS_VALIDITY_TERM;
    }

private:
    mutable bcos::SharedMutex x_state;
    std::string m_taskID;
    protocol::TaskStatus m_status;
    BsEcdhResult::Ptr m_result;
    std::atomic<uint64_t> m_latestActiveTime;
    uint64_t m_timeoutMinutes;
    uint64_t m_autoPauseThreshold{BS_VALIDITY_TERM};
    uint64_t activeCount{0};
};
}  // namespace ppc::psi
