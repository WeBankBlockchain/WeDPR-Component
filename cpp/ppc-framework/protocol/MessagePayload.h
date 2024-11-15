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
#include "ppc-framework/libwrapper/Buffer.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::protocol
{
enum class FrontMsgExtFlag : uint16_t
{
    Response = 0x1
};
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
    // for swig wrapper here
    virtual OutputBuffer dataBuffer() const
    {
        return OutputBuffer{(unsigned char*)m_dataPtr.data(), m_dataPtr.size()};
    }
    virtual void setData(bcos::bytes&& data)
    {
        m_data = std::move(data);
        m_dataPtr = bcos::bytesConstRef((bcos::byte*)m_data.data(), m_data.size());
    }
    virtual void setData(bcos::bytes const& data)
    {
        m_data = data;
        m_dataPtr = bcos::bytesConstRef((bcos::byte*)m_data.data(), m_data.size());
    }
    virtual void setDataPtr(bcos::bytesConstRef dataPtr) { m_dataPtr = dataPtr; }
    virtual bcos::bytesConstRef const& dataPtr() const { return m_dataPtr; }
    // the seq
    virtual uint16_t seq() const { return m_seq; }
    virtual void setSeq(uint16_t seq) { m_seq = seq; }
    // the length
    virtual int64_t length() const { return m_length; }

    // the traceID
    virtual std::string const& traceID() const { return m_traceID; }
    virtual void setTraceID(std::string const& traceID) { m_traceID = traceID; }

    virtual uint16_t ext() const { return m_ext; }
    virtual void setExt(uint16_t ext) { m_ext = ext; }

    virtual void setRespPacket() { m_ext |= (uint16_t)FrontMsgExtFlag::Response; }

    virtual bool isRespPacket() { return m_ext &= (uint16_t)FrontMsgExtFlag::Response; }

    void releasePayload()
    {
        m_data.clear();
        bcos::bytes().swap(m_data);
    }

protected:
    // the front payload version, used to support compatibility
    // Note: must init here to 0, otherwise, it will be unexpected value in some other platform
    uint8_t m_version = 0;
    // the seq
    uint16_t m_seq = 0;
    // the traceID
    std::string m_traceID;
    bcos::bytes m_data;
    // used to decrease the copy-overhead
    bcos::bytesConstRef m_dataPtr;
    uint16_t m_ext = 0;
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