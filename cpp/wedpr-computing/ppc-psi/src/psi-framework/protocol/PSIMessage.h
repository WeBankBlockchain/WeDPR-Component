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
 * @file PSIMessage.h
 * @author: yujiechen
 * @date 2022-11-9
 */
#pragma once
#include "../../Common.h"
#include "../interfaces/PSIMessageFactory.h"
#include "../interfaces/PSIMessageInterface.h"
#include "PSI.h"
#include <bcos-utilities/Error.h>

namespace ppc::psi
{
// Note: this implementation is not thread-safe
class PSIMessage : virtual public PSIMessageInterface
{
public:
    using Ptr = std::shared_ptr<PSIMessage>;
    PSIMessage() : m_inner([inner = ppctars::PSIMessage()]() mutable { return &inner; }) {}
    explicit PSIMessage(uint32_t _type) : PSIMessage() { setPacketType(_type); }
    explicit PSIMessage(bcos::bytesConstRef _data) : PSIMessage() { decode(_data); }
    explicit PSIMessage(std::function<ppctars::PSIMessage*()> _inner) : m_inner(std::move(_inner))
    {}
    ~PSIMessage();

    // the packet-type
    uint32_t packetType() const override { return m_inner()->packetType; }
    // the partyID(the current version is not used, but this field is added to facilitate other
    // things in the future (such as permission control, etc.))
    std::string const& partyID() const override { return m_inner()->partyID; }
    std::string const& resourceID() const override { return m_inner()->resourceID; }
    // for compatibility in the future
    int32_t version() const override { return m_inner()->version; }
    // get all the data: not recommend to use this method(only used for ut)
    uint64_t dataSize() const override { return m_inner()->data.size(); }

    std::vector<bcos::bytes> takeData() override
    {
        auto dataSize = m_inner()->data.size();
        std::vector<bcos::bytes> result;
        result.resize(dataSize);
        for (uint64_t i = 0; i < dataSize; i++)
        {
            std::move(m_inner()->data[i].begin(), m_inner()->data[i].end(),
                std::back_inserter(result[i]));
        }
        return result;
    }

    std::vector<long> const& dataIndex() const override { return m_inner()->dataIndex; }

    void resizeData(uint64_t size) override
    {
        m_inner()->data.resize(size);
        m_inner()->dataIndex.resize(size);
    }

    void setDataPair(uint64_t pos, uint64_t dataIndex, bcos::bytes const& data) override
    {
        m_inner()->dataIndex[pos] = dataIndex;
        m_inner()->data[pos].assign(data.begin(), data.end());
    }

    // Note: must ensure the object will not released before access the function
    bcos::bytesConstRef getData(uint64_t _index) override
    {
        auto const& dataItem = m_inner()->data.at(_index);
        return bcos::bytesConstRef((const bcos::byte*)(dataItem.data()), dataItem.size());
    }

    uint64_t getDataCount() const override { return m_inner()->data.size(); }
    void setPartyID(std::string const& _partyID) override { m_inner()->partyID = _partyID; }
    void setResourceID(std::string const& _resourceID) override
    {
        m_inner()->resourceID = _resourceID;
    }

    void setVersion(int32_t _version) override { m_inner()->version = _version; }
    void setData(std::vector<bcos::bytes> const& _data) override
    {
        setData(_data, 0, _data.size());
    }

    void setData(std::vector<bcos::bytes> const& _data, uint64_t _offset, uint64_t _len) override
    {
        m_inner()->data.clear();
        uint64_t endPos = std::min(_offset + _len, (uint64_t)_data.size());
        m_inner()->data.resize((endPos - _offset));
        uint64_t j = 0;
        for (uint64_t i = _offset; i < endPos; i++)
        {
            m_inner()->data[j].assign(_data.at(i).begin(), _data.at(i).end());
            j++;
        }
    }

    void setData(std::vector<bcos::bytes>&& _data) override
    {
        auto dataSize = _data.size();
        m_inner()->data.clear();
        m_inner()->data.resize(dataSize);
        for (uint64_t i = 0; i < dataSize; i++)
        {
            std::move(_data[i].begin(), _data[i].end(), std::back_inserter(m_inner()->data[i]));
        }
    }

    void constructData(std::vector<bcos::bytes> const& data, uint64_t startIndex) override
    {
        m_inner()->dataIndex.clear();
        int64_t i = startIndex;
        for (auto const& it : data)
        {
            m_inner()->dataIndex.emplace_back(i);
            i++;
        }
        setData(data);
    }

    void appendData(bcos::bytes const& _dataItem) override
    {
        m_inner()->data.emplace_back(_dataItem.begin(), _dataItem.end());
    }
    void setPacketType(uint32_t _packetType) override { m_inner()->packetType = _packetType; }

    void setDataBatchCount(uint32_t _dataBatchCount) override
    {
        m_inner()->dataBatchCount = _dataBatchCount;
    }
    uint32_t dataBatchCount() const override { return m_inner()->dataBatchCount; }

    // encode the PSIMessage
    bcos::bytesPointer encode() const override;
    // decode the PSIMessage
    void decode(bcos::bytesConstRef _data) override;

protected:
    std::function<ppctars::PSIMessage*()> m_inner;
};

class PSITaskNotificationMessageImpl : virtual public PSITaskNotificationMessage, public PSIMessage
{
public:
    using Ptr = std::shared_ptr<PSITaskNotificationMessageImpl>;
    PSITaskNotificationMessageImpl() : PSIMessage() {}
    explicit PSITaskNotificationMessageImpl(uint32_t _type) : PSIMessage(_type) {}
    explicit PSITaskNotificationMessageImpl(std::function<ppctars::PSIMessage*()> _inner)
      : PSIMessage(std::move(_inner))
    {}

    ~PSITaskNotificationMessageImpl() override = default;

    virtual void setErrorCode(int32_t _code) override { m_inner()->errorCode = _code; }
    virtual void setErrorMessage(std::string const& _errorMsg) override
    {
        m_inner()->errorMessage = _errorMsg;
    }

    virtual int32_t errorCode() const override { return m_inner()->errorCode; }
    virtual std::string const& errorMessage() const override { return m_inner()->errorMessage; }
};

class PSITaskInfoMsgImpl : public PSITaskInfoMsg, public PSIMessage
{
public:
    using Ptr = std::shared_ptr<PSITaskInfoMsgImpl>;
    PSITaskInfoMsgImpl() : PSIMessage() {}
    PSITaskInfoMsgImpl(uint32_t _type) : PSIMessage(_type) {}
    explicit PSITaskInfoMsgImpl(std::function<ppctars::PSIMessage*()> _inner)
      : PSIMessage(std::move(_inner))
    {}

    ~PSITaskInfoMsgImpl() override = default;

    std::vector<std::string> taskList() const override { return m_inner()->taskList; }

    void setTaskList(std::vector<std::string>&& _taskList) override
    {
        m_inner()->taskList = std::move(_taskList);
    }

    void setTaskList(std::vector<std::string> const& _taskList) override
    {
        m_inner()->taskList = _taskList;
    }
};

class PSIHandshakeRequestImpl : public PSIHandshakeRequest, public PSIMessage
{
public:
    using Ptr = std::shared_ptr<PSIHandshakeRequestImpl>;
    PSIHandshakeRequestImpl() : PSIMessage() {}
    PSIHandshakeRequestImpl(uint32_t _type) : PSIMessage(_type) {}
    PSIHandshakeRequestImpl(std::function<ppctars::PSIMessage*()> _inner)
      : PSIMessage(std::move(_inner))
    {}
    ~PSIHandshakeRequestImpl() override = default;

    void setCurves(std::vector<int>&& _curves) override { m_inner()->curves = std::move(_curves); }
    void setHashList(std::vector<int>&& _hashList) override
    {
        m_inner()->hashList = std::move(_hashList);
    }

    std::vector<int> const& curves() const override { return m_inner()->curves; }
    std::vector<int> const& hashList() const override { return m_inner()->hashList; }
};

// the psi handshake-response
class PSIHandshakeResponseImpl : public PSIHandshakeResponse, public PSIMessage
{
public:
    using Ptr = std::shared_ptr<PSIHandshakeResponseImpl>;
    PSIHandshakeResponseImpl() : PSIMessage() {}
    PSIHandshakeResponseImpl(uint32_t _type) : PSIMessage(_type) {}
    PSIHandshakeResponseImpl(std::function<ppctars::PSIMessage*()> _inner)
      : PSIMessage(std::move(_inner))
    {}

    ~PSIHandshakeResponseImpl() override = default;

    void setHashType(int _hashType) override { m_inner()->hashType = _hashType; }
    void setCurve(int _curve) override { m_inner()->curve = _curve; }

    // the selected hash algorithm
    int hashType() const override { return m_inner()->hashType; }
    // the selected curve
    int curve() const override { return m_inner()->curve; }
};

// the factory implementation
class PSIMessageFactoryImpl : virtual public PSIMessageFactory
{
public:
    using Ptr = std::shared_ptr<PSIMessageFactoryImpl>;
    PSIMessageFactoryImpl() = default;
    ~PSIMessageFactoryImpl() override {}

    PSIMessageInterface::Ptr createPSIMessage(uint32_t _packetType) override
    {
        return std::make_shared<PSIMessage>(_packetType);
    }

    PSITaskNotificationMessage::Ptr createTaskNotificationMessage(uint32_t _packetType) override
    {
        return std::make_shared<PSITaskNotificationMessageImpl>(_packetType);
    }

    PSITaskInfoMsg::Ptr createTaskInfoMessage(uint32_t _packetType) override
    {
        return std::make_shared<PSITaskInfoMsgImpl>(_packetType);
    }

    PSIHandshakeRequest::Ptr createHandshakeRequest(uint32_t _packetType) override
    {
        return std::make_shared<PSIHandshakeRequestImpl>(_packetType);
    }

    PSIHandshakeResponse::Ptr createHandshakeResponse(uint32_t _packetType) override
    {
        return std::make_shared<PSIHandshakeResponseImpl>(_packetType);
    }

protected:
    virtual PSIMessageInterface::Ptr decodePSIBaseMessage(
        std::function<ppctars::PSIMessage*()> _inner)
    {
        switch (_inner()->packetType)
        {
        case (uint32_t)PSIPacketType::PSIResultSyncMsg:
            return std::make_shared<PSIMessage>(_inner);
        case (uint32_t)PSIPacketType::CancelTaskNotification:
        case (uint32_t)PSIPacketType::PSIResultSyncResponse:
        case (uint32_t)PSIPacketType::HandshakeSuccess:
            return std::make_shared<PSITaskNotificationMessageImpl>(_inner);
        case (uint32_t)PSIPacketType::TaskSyncMsg:
            return std::make_shared<PSITaskInfoMsgImpl>(_inner);
        case (uint32_t)PSIPacketType::HandshakeRequest:
            return std::make_shared<PSIHandshakeRequestImpl>(_inner);
        case (uint32_t)PSIPacketType::HandshakeResponse:
            return std::make_shared<PSIHandshakeResponseImpl>(_inner);
        default:
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::UnknownPSIPacketType,
                "unknown psi-message-type " + std::to_string(_inner()->packetType)));
        }
    }
};
}  // namespace ppc::psi