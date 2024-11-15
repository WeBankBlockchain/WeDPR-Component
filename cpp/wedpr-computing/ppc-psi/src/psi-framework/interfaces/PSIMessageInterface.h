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
 * @file PSIMessageInterface.h
 * @author: yujiechen
 * @date 2022-11-9
 */
#pragma once
#include "ppc-framework/io/DataBatch.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::psi
{
// the base message
class PSIMessageInterface
{
public:
    using Ptr = std::shared_ptr<PSIMessageInterface>;
    PSIMessageInterface() = default;
    virtual ~PSIMessageInterface() = default;

    // the packet-type
    virtual uint32_t packetType() const = 0;
    // the partyID(the current version is not used, but this field is added to facilitate other
    // things in the future (such as permission control, etc.))
    virtual std::string const& partyID() const = 0;
    // the resourceID
    virtual std::string const& resourceID() const = 0;

    // for compatibility in the future
    virtual int32_t version() const = 0;
    // get all the data
    virtual std::vector<bcos::bytes> takeData() = 0;
    virtual void constructData(std::vector<bcos::bytes> const& data, uint64_t startIndex) = 0;
    virtual std::vector<long> const& dataIndex() const = 0;
    // Note: can't call this after takeData
    virtual uint64_t dataSize() const = 0;
    // get the data item
    // Note: must ensure the object will not released before access the function
    virtual bcos::bytesConstRef getData(uint64_t _index) = 0;
    virtual uint64_t getDataCount() const = 0;

    virtual void setPartyID(std::string const& _partyID) = 0;
    virtual void setResourceID(std::string const& _resourceID) = 0;
    virtual void setVersion(int32_t _version) = 0;
    virtual void setData(std::vector<bcos::bytes> const& _data) = 0;
    virtual void setData(std::vector<bcos::bytes>&& _data) = 0;
    virtual void setDataPair(uint64_t pos, uint64_t dataIndex, bcos::bytes const& data) = 0;
    virtual void resizeData(uint64_t size) = 0;
    // copy [_offset, _offset + _len) into the data-field
    virtual void setData(
        std::vector<bcos::bytes> const& _data, uint64_t _offset, uint64_t _len) = 0;

    virtual void appendData(bcos::bytes const& _dataItem) = 0;

    virtual void setPacketType(uint32_t _packetType) = 0;
    // the dataBatchSize
    virtual void setDataBatchCount(uint32_t _dataBatchCount) = 0;
    virtual uint32_t dataBatchCount() const = 0;
    // encode the PSIMessage
    virtual bcos::bytesPointer encode() const = 0;
    // decode the PSIMessage
    virtual void decode(bcos::bytesConstRef _data) = 0;

    // the field that should not serialized/deserialized, exist only in memory
    virtual void setTaskID(std::string const& _taskID) { m_taskID = _taskID; }
    virtual void setSeq(uint32_t _seq) { m_seq = _seq; }
    virtual void setFrom(std::string const& _from) { m_from = _from; }
    virtual void setFromNode(bcos::bytes const& fromNode) { m_fromNode = fromNode; }
    virtual bcos::bytes fromNode() const { return m_fromNode; }

    virtual std::string const& taskID() const { return m_taskID; }
    virtual uint32_t seq() const { return m_seq; }
    virtual std::string const& from() const { return m_from; }

    virtual std::string const& uuid() const { return m_uuid; }
    virtual void setUUID(std::string const& _uuid) { m_uuid = _uuid; }

private:
    std::string m_taskID;
    int32_t m_seq;
    // the agency
    std::string m_from;
    // the fromNode
    bcos::bytes m_fromNode;

    std::string m_uuid;
};


// the psi-task-notification message interface
class PSITaskNotificationMessage : virtual public PSIMessageInterface
{
public:
    using Ptr = std::shared_ptr<PSITaskNotificationMessage>;
    PSITaskNotificationMessage() = default;
    ~PSITaskNotificationMessage() override = default;

    virtual void setErrorCode(int32_t _code) = 0;
    virtual void setErrorMessage(std::string const& _errorMsg) = 0;

    virtual int32_t errorCode() const = 0;
    virtual std::string const& errorMessage() const = 0;
};

class PSITaskInfoMsg : virtual public PSIMessageInterface
{
public:
    using Ptr = std::shared_ptr<PSITaskInfoMsg>;
    PSITaskInfoMsg() = default;
    ~PSITaskInfoMsg() override = default;

    virtual std::vector<std::string> taskList() const = 0;
    virtual void setTaskList(std::vector<std::string>&& _taskList) = 0;
    virtual void setTaskList(std::vector<std::string> const& _taskList) = 0;
};

// the psi handshake-request
class PSIHandshakeRequest : virtual public PSIMessageInterface
{
public:
    using Ptr = std::shared_ptr<PSIHandshakeRequest>;
    PSIHandshakeRequest() = default;
    ~PSIHandshakeRequest() override = default;

    virtual void setCurves(std::vector<int>&& _curves) = 0;
    virtual void setHashList(std::vector<int>&& _hashList) = 0;

    virtual std::vector<int> const& curves() const = 0;
    virtual std::vector<int> const& hashList() const = 0;
};

// the psi handshake-response
class PSIHandshakeResponse : virtual public PSIMessageInterface
{
public:
    using Ptr = std::shared_ptr<PSIHandshakeResponse>;
    PSIHandshakeResponse() = default;
    ~PSIHandshakeResponse() override = default;

    virtual void setHashType(int _hashType) = 0;
    virtual void setCurve(int _curve) = 0;

    virtual int hashType() const = 0;
    virtual int curve() const = 0;
};

inline std::string printPSIMessage(PSIMessageInterface::Ptr _msg)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("from", _msg->from()) << LOG_KV("party", _msg->partyID())
                 << LOG_KV("task", _msg->taskID()) << LOG_KV("seq", _msg->seq())
                 << LOG_KV("resource", _msg->resourceID()) << LOG_KV("type", _msg->packetType());
    return stringstream.str();
}
}  // namespace ppc::psi