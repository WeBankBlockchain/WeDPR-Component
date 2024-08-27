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
 * @file MessagePayloadImpl.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#pragma once
#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/MessagePayload.h"

namespace ppc::protocol
{
class MessagePayloadImpl : public MessagePayload
{
public:
    using Ptr = std::shared_ptr<MessagePayloadImpl>;
    MessagePayloadImpl() = default;
    MessagePayloadImpl(bcos::bytesConstRef buffer) { decode(buffer); }
    ~MessagePayloadImpl() override {}

    int64_t encode(bcos::bytes& buffer) const override;
    int64_t decode(bcos::bytesConstRef data) override;

private:
    // version + seq + dataLen
    const unsigned int MIN_PAYLOAD_LEN = 6;
};


class MessagePayloadBuilderImpl : public MessagePayloadBuilder
{
public:
    using Ptr = std::shared_ptr<MessagePayloadBuilderImpl>;
    MessagePayloadBuilderImpl() = default;
    ~MessagePayloadBuilderImpl() override {}
    MessagePayload::Ptr build() override { return std::make_shared<MessagePayloadImpl>(); }
    MessagePayload::Ptr build(bcos::bytesConstRef buffer) override
    {
        return std::make_shared<MessagePayloadImpl>(buffer);
    }
};
}  // namespace ppc::protocol