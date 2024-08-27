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
 * @file MessagePayload.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#pragma once
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::protocol
{
class MessagePayload
{
public:
    using Ptr = std::shared_ptr<MessagePayload>;
    MessagePayload() = default;
    virtual ~MessagePayload() = default;

    virtual int64_t encode(bcos::bytes& buffer) const = 0;
    virtual int64_t decode(bcos::bytesConstRef data) = 0;

    // the version
    virtual uint8_t version() const { return m_version; }
    virtual void setVersion(uint8_t version) { m_version = version; }
    // data
    virtual bcos::bytes const& data() const { return m_data; }
    virtual void setData(bcos::bytes&& data) { m_data = std::move(data); }
    virtual void setData(bcos::bytes const& data) { m_data = data; }
    // the seq
    virtual uint16_t seq() const { return m_seq; }
    virtual void setSeq(uint16_t seq) { m_seq = seq; }
    // the length
    virtual int64_t length() const { return m_length; }

protected:
    // the front payload version, used to support compatibility
    uint8_t m_version;
    // the seq
    uint16_t m_seq;
    bcos::bytes m_data;
    int64_t mutable m_length;
};

class MessagePayloadBuilder
{
public:
    using Ptr = std::shared_ptr<MessagePayloadBuilder>;
    MessagePayloadBuilder() = default;
    virtual ~MessagePayloadBuilder() = default;
    virtual MessagePayload::Ptr build() = 0;
    virtual MessagePayload::Ptr build(bcos::bytesConstRef buffer) = 0;
};
}  // namespace ppc::protocol