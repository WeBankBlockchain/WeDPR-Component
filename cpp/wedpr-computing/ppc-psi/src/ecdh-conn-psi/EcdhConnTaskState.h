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
 * @file EcdhConnTaskState.h
 * @author: zachma
 * @date 2023-7-20
 */

#pragma once
#include "../psi-framework/TaskState.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/crypto/ECDHCrypto.h"

namespace ppc::psi
{
class EcdhConnTaskState : public TaskState
{
public:
    using Ptr = std::shared_ptr<EcdhConnTaskState>;
    EcdhConnTaskState(ppc::protocol::Task::ConstPtr const& _task,
        ppc::task::TaskResponseCallback&& _callback, bool _onlySelfRun = false)
      : TaskState(_task, std::move(_callback), _onlySelfRun)
    {}
    ~EcdhConnTaskState() = default;

    ppc::crypto::CryptoBox::Ptr cryptoBox() const
    {
        // bcos::ReadGuard l(x_cryptoBox);
        return m_cryptoBox;
    }

    void setCryptoBox(ppc::crypto::CryptoBox::Ptr const& _cryptoBox)
    {
        // bcos::WriteGuard l(x_cryptoBox);
        m_cryptoBox = _cryptoBox;
    }

    // return copy here to ensure thread-safe
    ppc::crypto::ECDHCrypto::Ptr ecdhCrypto() const
    {
        // bcos::ReadGuard l(x_ecdhCrypto);
        return m_ecdhCrypto;
    }
    // Note: here must lock, in-case of multiple tasks with the same task-id processed at almost the
    // same time
    void setEcdhCrypto(ppc::crypto::ECDHCrypto::Ptr const& _ecdhCrypto)
    {
        // bcos::WriteGuard l(x_ecdhCrypto);
        m_ecdhCrypto = _ecdhCrypto;
    }

private:
    ppc::crypto::ECDHCrypto::Ptr m_ecdhCrypto;
    // mutable bcos::SharedMutex x_ecdhCrypto;

    ppc::crypto::CryptoBox::Ptr m_cryptoBox;
    // mutable bcos::SharedMutex x_cryptoBox;
};

class EcdhConnTaskStateFactory : public TaskStateFactory
{
public:
    using Ptr = std::shared_ptr<EcdhConnTaskStateFactory>;
    EcdhConnTaskStateFactory() : TaskStateFactory() {}
    ~EcdhConnTaskStateFactory() override = default;

    EcdhConnTaskState::Ptr createConnTaskState(ppc::protocol::Task::ConstPtr const& _task,
        ppc::task::TaskResponseCallback&& _callback, bool _onlySelfRun = false,
        PSIConfig::Ptr _config = nullptr)
    {
        return std::make_shared<EcdhConnTaskState>(_task, std::move(_callback), false);
    }
};

}  // namespace ppc::psi