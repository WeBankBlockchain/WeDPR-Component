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
 * @file EcdhPSIImpl.h
 * @author: yujiechen
 * @date 2022-12-27
 */
#pragma once
#include "../psi-framework/PSIFramework.h"
#include "EcdhCache.h"
#include "EcdhPSIConfig.h"
#include "EcdhTaskState.h"

namespace ppc::psi
{
class EcdhPSIImpl : public PSIFramework, public std::enable_shared_from_this<EcdhPSIImpl>
{
public:
    using Ptr = std::shared_ptr<EcdhPSIImpl>;
    EcdhPSIImpl(EcdhPSIConfig::Ptr const& _config, unsigned _idleTimeMs = 0)
      : PSIFramework(
            _config->msgFactory(), _config->dataResourceLoader(), _config, "ecdh-psi", _idleTimeMs),
        m_config(_config)
    {
        m_taskStateFactory = std::make_shared<EcdhTaskStateFactory>();
        m_cache = std::make_shared<EcdhCache>(_config);
    }

    ~EcdhPSIImpl() override = default;

    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    // handle the psi message
    void handlePSIMsg(PSIMessageInterface::Ptr _msg) override;
    // should lock the party resource or not
    bool needLockResource(int, int _partyType) override
    {
        return _partyType == (int)ppc::protocol::PartyType::Client;
    }

protected:
    // receive the handshake response
    void onHandshakeResponse(PSIMessageInterface::Ptr const& _msg) override;
    // receive the handshake request
    void onHandshakeRequest(PSIMessageInterface::Ptr const& _msg) override;

    // the psi-server and psi-client load data from DataResource and blind the plainData into cipher
    virtual void blindData(TaskState::Ptr const& _taskState);
    virtual void triggerDataBlind(TaskState::Ptr const& _taskState);

    // receive the blinded-data from client, the psi-server response the evaluated data
    virtual void handleEvaluateRequest(PSIMessageInterface::Ptr _msg);
    // receive the evaluate-response from the sever
    virtual void handleEvaluateResponse(PSIMessageInterface::Ptr _msg);

    // receive the blinded-data from the server
    virtual void handleServerBlindData(PSIMessageInterface::Ptr _msg);
    // receive the SyncDataBatchCount request from the server
    virtual void onRecvSyncDataInfoMsg(PSIMessageInterface::Ptr const& _msg);

    void runPSI(TaskState::Ptr const& _taskState);

    int selectCryptoAlgorithm(
        std::vector<int> _clientSupportedAlgorithms, std::set<int> _localSupportedAlgorithms);

    // init the task-state
    bool initTaskState(TaskState::Ptr const& _taskState);

    std::pair<TaskState::Ptr, ppc::crypto::ECDHCrypto::Ptr> checkAndObtainTaskState(
        PSIMessageInterface::Ptr _msg);

protected:
    EcdhPSIConfig::Ptr m_config;
    EcdhCache::Ptr m_cache;
};
}  // namespace ppc::psi