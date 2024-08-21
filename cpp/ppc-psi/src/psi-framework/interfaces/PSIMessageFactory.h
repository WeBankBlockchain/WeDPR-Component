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
 * @file PSIMessageFactory.h
 * @author: yujiechen
 * @date 2022-11-9
 */
#pragma once
#include "PSIMessageInterface.h"
#include <memory>
namespace ppc::psi
{
class PSIMessageFactory
{
public:
    using Ptr = std::shared_ptr<PSIMessageFactory>;
    PSIMessageFactory() = default;
    virtual ~PSIMessageFactory() = default;

    virtual PSIMessageInterface::Ptr decodePSIMessage(bcos::bytesConstRef _data) = 0;
    virtual PSIMessageInterface::Ptr createPSIMessage(uint32_t _packetType) = 0;

    virtual PSIHandshakeRequest::Ptr createHandshakeRequest(uint32_t _packetType) = 0;
    virtual PSIHandshakeResponse::Ptr createHandshakeResponse(uint32_t _packetType) = 0;

    virtual PSITaskNotificationMessage::Ptr createTaskNotificationMessage(uint32_t _packetType) = 0;
    virtual PSITaskInfoMsg::Ptr createTaskInfoMessage(uint32_t _packetType) = 0;
};
}  // namespace ppc::psi