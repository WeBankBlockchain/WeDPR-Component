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

    virtual void onReceivedErrorNotification(const std::string& _taskID){};
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
        bool _enforceSelfResource = true)
    {
        {
            // check task id
            bcos::ReadGuard l(x_pendingTasks);
            if (m_pendingTasks.contains(_task->id()))
            {
                return std::make_shared<bcos::Error>(
                    (int)PSIRetCode::DuplicatedTask, "task already exists");
            }
        }

        // check self party
        auto const& selfParty = _task->selfParty();
        if (!selfParty)
        {
            return std::make_shared<bcos::Error>(
                (int)PSIRetCode::TaskParamsError, "must specify self party info");
        }
        if (selfParty->partyIndex() == (uint16_t)protocol::PartyType::Client)
        {
            _enforceSelfInput = true;
            _enforceSelfOutput = true;
        }

        // check self data resource
        if (_enforceSelfResource)
        {
            auto const& dataResource = selfParty->dataResource();
            if (!dataResource)
            {
                return std::make_shared<bcos::Error>(
                    (int)PSIRetCode::TaskParamsError, "no data resource specified for self party");
            }
            auto originData = dataResource->rawData();
            if (_enforceSelfInput && originData.empty() && !dataResource->desc())
            {
                return std::make_shared<bcos::Error>(
                    (int)PSIRetCode::TaskParamsError, "no input resource specified for self party");
            }
            if (_enforceSelfOutput && originData.empty() && !dataResource->outputDesc())
            {
                return std::make_shared<bcos::Error>((int)PSIRetCode::TaskParamsError,
                    "no output resource specified for self party");
            }
        }

        // check party index
        std::set<uint16_t> indexSet;
        auto index = selfParty->partyIndex();
        if (_partiesCount > 1 && index >= _partiesCount)
        {
            return std::make_shared<bcos::Error>(
                (int)PSIRetCode::TaskParamsError, "invalid partyIndex: " + std::to_string(index));
        }
        indexSet.insert(index);

        // check the peer size
        auto const& peerParties = _task->getAllPeerParties();
        if (peerParties.size() != uint(_partiesCount - 1))
        {
            std::string errorMessage = "expected parties count: " + std::to_string(_partiesCount) +
                                       ", current is " + std::to_string(peerParties.size());
            return std::make_shared<bcos::Error>((int)PSIRetCode::TaskParamsError, errorMessage);
        }

        // check the party index of peers
        for (auto& it : peerParties)
        {
            index = it.second->partyIndex();
            if (_partiesCount > 1 && index >= _partiesCount)
            {
                return std::make_shared<bcos::Error>((int)PSIRetCode::TaskParamsError,
                    "invalid partyIndex: " + std::to_string(index));
            }
            if (indexSet.contains(index))
            {
                return std::make_shared<bcos::Error>((int)PSIRetCode::TaskParamsError,
                    "repeated party index: " + std::to_string(index));
            }
            indexSet.insert(index);
        }

        // check the data resource of peer
        if (!_enforcePeerResource)
        {
            return nullptr;
        }
        for (auto& it : peerParties)
        {
            if (!it.second->dataResource())
            {
                return std::make_shared<bcos::Error>((int)PSIRetCode::NotSpecifyPeerDataResource,
                    "must specify the peer data resource");
            }
        }
        return nullptr;
    }

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

    void noticePeerToFinish(const ppc::protocol::Task::ConstPtr& _task)
    {
        auto const& peerParties = _task->getAllPeerParties();
        if (peerParties.empty())
        {
            return;
        }

        for (const auto& peer : peerParties)
        {
            noticePeerToFinish(_task->id(), peer.first);
        }
    }

    void noticePeerToFinish(const std::string& _taskID, const std::string& _peer)
    {
        PSI_LOG(INFO) << LOG_DESC("noticePeerToFinish") << LOG_KV("task", _taskID)
                      << LOG_KV("peer", _peer);
        try
        {
            if (_peer.empty())
            {
                return;
            }

            auto message =
                m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
                    uint8_t(m_type), _taskID, std::make_shared<bcos::bytes>());
            message->setMessageType(uint8_t(CommonMessageType::ErrorNotification));
            m_config->front()->asyncSendMessage(
                _peer, message, m_config->networkTimeout(), [](const bcos::Error::Ptr&) {},
                nullptr);
        }
        catch (std::exception& e)
        {
            PSI_LOG(ERROR) << LOG_DESC("noticePeerToFinish")
                           << LOG_KV("exception", boost::diagnostic_information(e));
        }
    }

    void checkPeerActivity()
    {
        bcos::ReadGuard l(x_pendingTasks);
        for (auto const& it : m_pendingTasks)
        {
            if (it.second->onlySelfRun())
            {
                // no need to check peer
                continue;
            }

            auto task = it.second->task();
            auto const& peerParties = task->getAllPeerParties();
            if (peerParties.empty())
            {
                continue;
            }

            for (const auto& peer : peerParties)
            {
                auto message =
                    m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
                        uint8_t(m_type), task->id(), std::make_shared<bcos::bytes>());
                message->setMessageType(uint8_t(CommonMessageType::PingPeer));
                m_config->front()->asyncSendMessage(
                    peer.first, message, m_config->networkTimeout(),
                    [this, task, peerID = peer.first](bcos::Error::Ptr&& _error) {
                        if (!_error || _error->errorCode() == 0)
                        {
                            return;
                        }
                        onSelfError(task->id(),
                            std::make_shared<bcos::Error>(
                                (int)PSIRetCode::PeerNodeDown, "peer node is down, id: " + peerID),
                            false);
                    },
                    nullptr);
            }
        }
        m_pingTimer->restart();
    }

    io::LineReader::Ptr loadReader(std::string const& _taskID,
        protocol::DataResource::ConstPtr const& _dataResource, io::DataSchema _dataSchema,
        uint32_t _columnSize = 1)
    {
        if (!_dataResource->rawData().empty())
        {
            return nullptr;
        }
        auto reader =
            m_config->dataResourceLoader()->loadReader(_dataResource->desc(), _dataSchema, true);

        if (reader->columnSize() == 0 || (reader->type() == protocol::DataResourceType::MySQL &&
                                             reader->columnSize() != _columnSize))
        {
            auto errorMsg = "load data for task " + _taskID + "failed, expect " +
                            std::to_string(_columnSize) + " column, current column size is " +
                            std::to_string(reader->columnSize());
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::LoadDataFailed, errorMsg));
        }
        return reader;
    }

    io::LineWriter::Ptr loadWriter(std::string const& _taskID,
        protocol::DataResource::ConstPtr const& _dataResource, bool _enableOutputExists)
    {
        if (!_dataResource->rawData().empty())
        {
            return nullptr;
        }
        if (!_enableOutputExists)
        {
            // Note: if the output-resource already exists, will throw exception
            m_config->dataResourceLoader()->checkResourceExists(_dataResource->outputDesc());
        }
        return m_config->dataResourceLoader()->loadWriter(_dataResource->outputDesc(), true);
    }

    TaskState::Ptr findPendingTask(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_pendingTasks);
        auto it = m_pendingTasks.find(_taskID);
        if (it == m_pendingTasks.end())
        {
            return nullptr;
        }
        return it->second;
    }

    void addPendingTask(TaskState::Ptr _taskState)
    {
        bcos::WriteGuard l(x_pendingTasks);
        auto id = _taskState->task()->id();
        m_pendingTasks[id] = std::move(_taskState);
    }

    void removePendingTask(const std::string& _taskID)
    {
        bcos::WriteGuard l(x_pendingTasks);
        auto it = m_pendingTasks.find(_taskID);
        if (it != m_pendingTasks.end())
        {
            m_pendingTasks.erase(it);
        }
    }

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