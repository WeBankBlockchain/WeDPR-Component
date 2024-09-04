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
 * @file BSECDHPSIInterface.h
 * @author: shawnhe
 * @date 2023-09-20
 */
#pragma once
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-psi/src/bs-ecdh-psi/protocol/BsEcdhResult.h"
#include "ppc-psi/src/bs-ecdh-psi/protocol/Message.h"
#include <bcos-utilities/Error.h>
#include <memory>

namespace ppc::psi
{
class BsEcdhPSIInterface
{
public:
    using Ptr = std::shared_ptr<BsEcdhPSIInterface>;

    BsEcdhPSIInterface() = default;

    virtual ~BsEcdhPSIInterface() = default;

    virtual BsEcdhResult::Ptr getTaskStatus(GetTaskStatusRequest::Ptr _request) = 0;

    virtual BsEcdhResult::Ptr updateTaskStatus(UpdateTaskStatusRequest::Ptr _request) = 0;

    virtual BsEcdhResult::Ptr killTask(KillTaskRequest::Ptr _request) = 0;

    virtual void asyncRunTask(RunTaskRequest::Ptr _request,
        std::function<void(BsEcdhResult::Ptr&&)>&& _onTaskFinished) = 0;

    virtual BsEcdhResult::Ptr fetchCipher(FetchCipherRequest::Ptr _request) = 0;

    virtual BsEcdhResult::Ptr sendEcdhCipher(SendEcdhCipherRequest::Ptr _request) = 0;

    virtual BsEcdhResult::Ptr sendPartnerCipher(SendPartnerCipherRequest::Ptr _request) = 0;

    virtual void start() = 0;

    virtual void stop() = 0;
};
}  // namespace ppc::psi