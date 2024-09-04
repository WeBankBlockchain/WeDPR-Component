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
 * @file EcdhPSIMessageFactory.h
 * @author: yujiechen
 * @date 2022-12-29
 */
#pragma once
#include "../psi-framework/protocol/PSIMessage.h"
namespace ppc::psi
{
class EcdhPSIMessageFactory : public PSIMessageFactoryImpl
{
public:
    using Ptr = std::shared_ptr<EcdhPSIMessageFactory>;
    EcdhPSIMessageFactory() = default;
    ~EcdhPSIMessageFactory() override = default;

    PSIMessageInterface::Ptr decodePSIMessage(bcos::bytesConstRef _data) override
    {
        auto inner = [inner = ppctars::PSIMessage()]() mutable { return &inner; };
        tars::TarsInputStream<tars::BufferReader> input;
        input.setBuffer((const char*)_data.data(), _data.size());
        inner()->readFrom(input);
        switch (inner()->packetType)
        {
        case (uint32_t)ECDHPacketType::EvaluateRequest:
        case (uint32_t)ECDHPacketType::EvaluateResponse:
        case (uint32_t)ECDHPacketType::ServerBlindedData:
        case (uint32_t)ECDHPacketType::SyncDataBatchInfo:
            return std::make_shared<PSIMessage>(inner);
        default:
            return decodePSIBaseMessage(inner);
        }
    }
};
}  // namespace ppc::psi