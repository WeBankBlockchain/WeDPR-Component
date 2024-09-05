/*
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
 * @file EcdhTaskState.h
 * @author: yujiechen
 * @date 2023-1-3
 */
#pragma once
#include "../psi-framework/TaskState.h"
#include "ppc-framework/crypto/ECDHCrypto.h"
namespace ppc::psi
{
class EcdhTaskState : public TaskState
{
public:
    using Ptr = std::shared_ptr<EcdhTaskState>;
    EcdhTaskState(ppc::protocol::Task::ConstPtr const& _task,
        ppc::task::TaskResponseCallback&& _callback, bool _onlySelfRun = false)
      : TaskState(_task, std::move(_callback), _onlySelfRun)
    {}
    ~EcdhTaskState() override = default;

    // return copy here to ensure thread-safe
    ppc::crypto::ECDHCrypto::Ptr ecdhCrypto() const
    {
        bcos::ReadGuard l(x_ecdhCrypto);
        return m_ecdhCrypto;
    }
    // Note: here must lock, in-case of multiple tasks with the same task-id processed at almost the
    // same time
    void setEcdhCrypto(ppc::crypto::ECDHCrypto::Ptr const& _ecdhCrypto)
    {
        bcos::WriteGuard l(x_ecdhCrypto);
        m_ecdhCrypto = _ecdhCrypto;
    }

private:
    ppc::crypto::ECDHCrypto::Ptr m_ecdhCrypto;
    mutable bcos::SharedMutex x_ecdhCrypto;
};

class EcdhTaskStateFactory : public TaskStateFactory
{
public:
    using Ptr = std::shared_ptr<EcdhTaskStateFactory>;
    EcdhTaskStateFactory() : TaskStateFactory() {}
    ~EcdhTaskStateFactory() override = default;

    TaskState::Ptr createTaskState(ppc::protocol::Task::ConstPtr const& _task,
        ppc::task::TaskResponseCallback&& _callback, bool _onlySelfRun = false,
        PSIConfig::Ptr _config = nullptr) override
    {
        return std::make_shared<EcdhTaskState>(_task, std::move(_callback), false);
    }
};
}  // namespace ppc::psi