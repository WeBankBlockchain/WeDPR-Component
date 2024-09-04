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
 * @file PSIConnMessage.h
 * @author: zachma
 * @date 2023-8-23
 */
#pragma once
#include "ppc-framework/io/DataBatch.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::psi
{
class PSIConnMessage
{
public:
    using Ptr = std::shared_ptr<PSIConnMessage>;
    PSIConnMessage(const std::string& _key) : m_key(_key){};
    ~PSIConnMessage() = default;

    // the key
    std::string const& key() const { return m_key; }
    void setKey(const std::string& _key) { m_key = _key; }

    // the value
    bcos::bytes value() { return m_value; }
    void setValue(bcos::bytes _value) { m_value = _value; }

    // the packetType
    uint8_t packetType() { return m_packetType; }
    void setPacketType(uint8_t _packetType) { m_packetType = _packetType; }

    // the sender
    uint16_t sender() { return m_sender; }
    void setSender(uint16_t _sender) { m_sender = _sender; }

    // the receiver
    uint16_t receiver() { return m_receiver; }
    void setReceiver(uint16_t _receiver) { m_receiver = _receiver; }

    // the MainProcess
    uint8_t mainProcess() { return m_mainProcess; }
    void setMainProcess(uint8_t _mainProcess) { m_mainProcess = _mainProcess; }

    // the SubProcess
    uint8_t subProcess() { return m_subProcess; }
    void setSubProcess(uint8_t _subProcess) { m_subProcess = _subProcess; }

    // the TaskId
    std::string const& taskID() const { return m_taskId; }
    void setTaskID(const std::string& _taskId) { m_taskId = _taskId; }

private:
    std::string m_key;
    std::string m_taskId;
    bcos::bytes m_value;
    uint8_t m_packetType;
    uint16_t m_sender;
    uint16_t m_receiver;
    uint8_t m_subProcess;
    uint8_t m_mainProcess;
};

class HandShakeRequestVo
{
public:
    using Ptr = std::shared_ptr<HandShakeRequestVo>;
    HandShakeRequestVo() = default;
    HandShakeRequestVo(std::set<int32_t> curve, std::set<int32_t> hash, int32_t protocol_families,
        int32_t item_count)
      : m_curve(curve),
        m_hash(hash),
        m_protocol_families(protocol_families),
        m_item_count(item_count)
    {}

    ~HandShakeRequestVo() = default;
    void SetCurve(const std::set<int32_t>& curve) { m_curve = curve; }

    void SetHash(const std::set<int32_t>& hash) { m_hash = hash; }

    void SetProtocolFamilies(int32_t protocol_families) { m_protocol_families = protocol_families; }

    void SetItemCount(int32_t item_count) { m_item_count = item_count; }

    std::set<int32_t> GetCurve() const { return m_curve; }

    std::set<int32_t> GetHash() const { return m_hash; }

    int32_t GetProtocolFamilies() const { return m_protocol_families; }

    int32_t GetItemCount() const { return m_item_count; }

private:
    std::set<int32_t> m_curve;
    std::set<int32_t> m_hash;
    int32_t m_protocol_families;
    int32_t m_item_count;
};

class HandShakeResponseVo
{
public:
    using Ptr = std::shared_ptr<HandShakeResponseVo>;
    HandShakeResponseVo() = default;
    HandShakeResponseVo(int32_t protocol_families, int32_t curve, int32_t hash)
      : m_protocol_families(protocol_families), m_curve(curve), m_hash(hash)
    {}

    virtual ~HandShakeResponseVo() = default;

    void SetCurve(int32_t curve) { m_curve = curve; }

    void SetHash(int32_t hash) { m_hash = hash; }

    void SetProtocolFamilies(int32_t protocol_families) { m_protocol_families = protocol_families; }

    void SetErrorCode(int32_t error_code) { m_error_code = error_code; }

    void SetErrorMessage(const std::string& msg) { m_error_msg = msg; }

    int32_t GetCurve() const { return m_curve; }

    int32_t GetHash() const { return m_hash; }

    int32_t GetProtocolFamilies() const { return m_protocol_families; }

    int32_t GetErrorCode() const { return m_error_code; }

    std::string GetErrorMsg() const { return m_error_msg; }

private:
    int32_t m_curve;
    int32_t m_hash;
    int32_t m_protocol_families;
    int32_t m_error_code;
    std::string m_error_msg;
};

class CipherBatchVo
{
public:
    using Ptr = std::shared_ptr<CipherBatchVo>;
    CipherBatchVo() = default;
    CipherBatchVo(std::string _type, int32_t _batch_index, bool _is_last_batch)
      : m_type(_type), m_batch_index(_batch_index), m_is_last_batch(_is_last_batch)
    {}
    virtual ~CipherBatchVo() = default;

    void setType(const std::string& _type) { m_type = _type; }

    void setBatchIndex(int32_t _batch_index) { m_batch_index = _batch_index; }

    void setIsLastBatch(bool _is_last_batch) { m_is_last_batch = _is_last_batch; }

    void setCount(int32_t _count) { m_count = _count; }

    void setCipherText(const std::vector<bcos::bytes>& _cipherText) { m_ciphertext = _cipherText; }

    std::string type() const { return m_type; }

    int32_t batch_index() const { return m_batch_index; }

    bool is_last_batch() const { return m_is_last_batch; }

    int32_t count() const { return m_count; }

    std::vector<bcos::bytes> cipherText() const { return m_ciphertext; }

private:
    std::string m_type;
    int32_t m_batch_index;
    bool m_is_last_batch;
    int32_t m_count;
    std::vector<bcos::bytes> m_ciphertext;
};


inline std::string printPSIConnMessage(PSIConnMessage::Ptr _msg)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("sender", _msg->sender()) << LOG_KV("key", _msg->key())
                 << LOG_KV("receive", _msg->receiver()) << LOG_KV("taskId", _msg->taskID())
                 << LOG_KV("message type", _msg->packetType())
                 << LOG_KV("main process", _msg->mainProcess())
                 << LOG_KV("sub process", _msg->subProcess());
    return stringstream.str();
}
}  // namespace ppc::psi