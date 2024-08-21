/**
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
 * @file EcdhConnPSIClient.cpp
 * @author: zachma
 * @date 2023-7-18
 */
#include "EcdhConnPSIClient.h"
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/DataConvertUtility.h>

using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace bcos;

EcdhConnPSIClient::EcdhConnPSIClient(
    EcdhConnPSIConfig::Ptr _config, EcdhConnTaskState::Ptr _taskState)
  : m_config(_config), m_taskState(_taskState), m_send_count(0)
{
    auto task = m_taskState->task();
    m_taskID = task->id();
    m_msgFactory = std::make_shared<PSIConnMessageFactory>();
}

void EcdhConnPSIClient::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    ECDH_CONN_LOG(INFO) << LOG_DESC("asyncStartRunTask: ") << printTaskInfo(_task);
    m_header = initHeader(_task->id());
    sendHandshakeRequest(m_taskState);
}

void EcdhConnPSIClient::onSecondCipherServerToClient(const bcos::bytes& _msg)
{
    // 接收Server发来的M_Q2
    auto cipherExVo = m_msgFactory->parseCipherExchange(_msg);
    m_Q2 = cipherExVo->cipherText();

    auto intersectionVec = tryIntersection(m_Q2, m_P2_Client);
    m_taskState->storePSIResult(m_config->dataResourceLoader(), intersectionVec);

    // 把m_P2_Client发送到Server
    // create CipherExchange
    auto cipherBatchVo = std::make_shared<CipherBatchVo>("dual.enc", 1, true);
    cipherBatchVo->setCount(m_P2_Client.size());
    cipherBatchVo->setCipherText(m_P2_Client);
    auto cipherBytes = m_msgFactory->createCipherExchange(cipherBatchVo);
    auto requestBytes = m_msgFactory->createPSIConnMessageRequest(
        cipherBytes, generateKey((int)EcdhConnSubProcess::CipherSecondProcess, 0));

    auto message = m_config->ppcMsgFactory()->buildPPCMessage(m_taskState->task()->type(),
        m_taskState->task()->algorithm(), m_taskID, std::make_shared<bcos::bytes>(requestBytes));
    message->setSender(m_taskState->task()->selfParty()->id());
    message->setHeader(m_header);
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [this, self = weak_from_this()](bcos::Error::Ptr _error) {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            if (!_error)
            {
                m_taskState->onTaskFinished();
                return;
            }
        },
        nullptr);
}

void EcdhConnPSIClient::onCipherServerToClient(const bcos::bytes& _msg)
{
    // parse EcdhPsiCipherBatch to vector
    auto cipherExVo = m_msgFactory->parseCipherExchange(_msg);
    m_P1_Client = cipherExVo->cipherText();
    ECDH_CONN_LOG(INFO) << LOG_DESC("onCipherServerToClient: ")
                        << LOG_KV("m_P1_Client SIZE: ", m_P1_Client.size());

    computeAndEncryptSetSecond(m_P1_Client, std::make_shared<bcos::bytes>(m_randomA));


    auto cipherBatchVo = std::make_shared<CipherBatchVo>("enc", 0, true);

    cipherBatchVo->setCount(m_Q1.size());
    cipherBatchVo->setCipherText(m_Q1);
    auto cipherBytes = m_msgFactory->createCipherExchange(cipherBatchVo);
    auto requestBytes = m_msgFactory->createPSIConnMessageRequest(
        cipherBytes, generateKey((int)EcdhConnProcess::CipherProcess, -1));


    auto message = m_config->ppcMsgFactory()->buildPPCMessage(m_taskState->task()->type(),
        m_taskState->task()->algorithm(), m_taskID, std::make_shared<bcos::bytes>(requestBytes));
    message->setSender(m_taskState->task()->selfParty()->id());
    message->setHeader(m_header);

    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [this, self = weak_from_this()](bcos::Error::Ptr _error) {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            if (!_error)
            {
                return;
            }
        },
        nullptr);
}

void EcdhConnPSIClient::onHandShakeResponseHandler(const bcos::bytes& _msg)
{
    ECDH_CONN_LOG(INFO) << LOG_DESC("onHandShakeResponseHandler start");
    auto handShakeResponseVo = m_msgFactory->parseHandshakeResponse(_msg);
    auto errCode = handShakeResponseVo->GetErrorCode();
    auto errMsg = handShakeResponseVo->GetErrorMsg();

    if (errCode)
    {
        auto errorMsg = "onHandShakeResponseHandler failed for no errCode/errMsg selected";
        ECDH_CONN_LOG(WARNING) << LOG_DESC(errorMsg) << LOG_KV("errCode", errCode)
                               << LOG_KV("errMsg", errMsg);
        auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandshakeFailed, errorMsg);
        result->setError(std::move(error));
        m_taskState->onTaskFinished(result, false);
        return;
    }

    auto curve = handShakeResponseVo->GetCurve();
    auto hashType = handShakeResponseVo->GetHash();
#ifdef ENABLE_CONN
    auto cryptoBox = m_config->ecdhCryptoFactory()->createCryptoBox(
        (int)ppc::protocol::ECCCurve::SM2, (int)ppc::protocol::HashImplName::SHA256);
#else
    auto cryptoBox =
        m_config->ecdhCryptoFactory()->createCryptoBox((int8_t)curve, (int8_t)hashType);
#endif
    m_taskState->setCryptoBox(cryptoBox);
    ECDH_CONN_LOG(INFO) << LOG_DESC("onHandShakeResponseHandler success");
    // CREATE RANDOM
    m_randomA = m_taskState->cryptoBox()->eccCrypto()->generateRandomScalar();
    computeAndEncryptSet(std::make_shared<bcos::bytes>(m_randomA));
}

void EcdhConnPSIClient::computeAndEncryptSet(bcos::bytesPointer _randA)
{
    auto inputSize = m_originInputs->size();
    if (!m_originInputs || inputSize == 0)
    {
        ECDH_CONN_LOG(ERROR) << LOG_DESC("Client computeAndEncryptSet")
                             << LOG_DESC("data size is empty!");
        auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
        auto err = std::make_shared<bcos::Error>(-12222, "canceled by other peer");
        result->setError(std::move(err));
        m_taskState->onTaskFinished(result, false);
    }
    std::vector<bcos::bytes> _res_vectors;
    auto hash = m_taskState->cryptoBox()->hashImpl();
    for (int i = 0; i < inputSize; i++)
    {
        auto data = m_originInputs->getBytes(i);
        auto hashData = hash->hash(bcos::bytesConstRef(data.data(), data.size()));
        auto point = m_taskState->cryptoBox()->eccCrypto()->hashToCurve(hashData);
        auto hashSet = m_taskState->cryptoBox()->eccCrypto()->ecMultiply(point, *_randA);
        _res_vectors.push_back(hashSet);
    }

    m_Q1 = _res_vectors;
    ECDH_CONN_LOG(INFO) << LOG_DESC("computeAndEncryptSet success")
                        << LOG_KV("m_Q1 size: ", m_Q1.size());
}

void EcdhConnPSIClient::computeAndEncryptSetSecond(
    const std::vector<bcos::bytes>& _input, bcos::bytesPointer _randA)
{
    std::vector<bcos::bytes> _res_vectors;
    auto inputSize = _input.size();
    for (int i = 0; i < inputSize; i++)
    {
        auto inputData = _input.at(i);
        auto encryptSet = m_taskState->cryptoBox()->eccCrypto()->ecMultiply(inputData, *_randA);
        _res_vectors.push_back(encryptSet);
    }

    ECDH_CONN_LOG(INFO) << LOG_DESC("onCipherServerToClient: computeAndEncryptSetSecond: ")
                        << LOG_KV("m_P1_Client SIZE: ", m_P2_Client.size());

    m_P2_Client = _res_vectors;
}


void EcdhConnPSIClient::sendHandshakeRequest(TaskState::Ptr _taskState)
{
    if (_taskState->peerID().empty())
    {
        return;
    }
    ECDH_CONN_LOG(TRACE) << LOG_DESC("sendHandshakeRequest start");
    m_originInputs = _taskState->loadAllData();
    auto item_count = m_originInputs->size();
    auto ecdhConnPSIConfig = m_config->ppcConfig();
    auto handReqVo = std::make_shared<HandShakeRequestVo>();
    std::set<int32_t> _curve{(int32_t)ECCCurve::P256};
    std::set<int32_t> _hash{(int32_t)HashImplName::SHA256};
    handReqVo->SetCurve(_curve);
    handReqVo->SetHash(_hash);
    handReqVo->SetItemCount(item_count);
    auto handshakeBytes = m_msgFactory->createHandshakeRequest(handReqVo);
    auto key = generateKey((int)EcdhConnProcess::HandShakeProcess, -1);
    auto requestBytes = m_msgFactory->createPSIConnMessageRequest(handshakeBytes, key);

    auto message = m_config->ppcMsgFactory()->buildPPCMessage(_taskState->task()->type(),
        _taskState->task()->algorithm(), m_taskID, std::make_shared<bcos::bytes>(requestBytes));
    message->setSender(_taskState->task()->selfParty()->id());
    message->setHeader(m_header);
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](bcos::Error::Ptr _error) {
            if (!_error)
            {
                return;
            }
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
        },
        nullptr);
}

// input1 ∩ input2 ->
std::vector<bcos::bytes> EcdhConnPSIClient::tryIntersection(const std::vector<bcos::bytes>& _input1,
    const std::vector<bcos::bytes>& _input2, int _compareSuffixBitSize)
{
    std::vector<bcos::bytes> input1;
    for (auto const& _in : _input1)
    {
        bcos::bytes::const_iterator head = _in.begin() + _in.size() - _compareSuffixBitSize;
        bcos::bytes input_temp(head, _in.end());
        input1.push_back(input_temp);
        ECDH_CONN_LOG(TRACE) << LOG_KV("input_temp VALUE:", *toHexString(input_temp))
                             << LOG_KV("input_temp size:", input_temp.size());
    }

    std::vector<bcos::bytes> input2;
    for (auto const& _in : _input2)
    {
        bcos::bytes::const_iterator head = _in.begin() + _in.size() - _compareSuffixBitSize;
        bcos::bytes input_temp(head, _in.end());
        input2.push_back(input_temp);
        ECDH_CONN_LOG(TRACE) << LOG_KV("input_temp VALUE:", *toHexString(input_temp))
                             << LOG_KV("input_temp size:", input_temp.size());
    }

    int index = 0;
    std::set<bcos::bytes> result_set;
    for (auto const& _in : input1)
    {
        if (std::find(input2.begin(), input2.end(), _in) != input2.end())
        {
            result_set.insert(m_originInputs->getBytes(index));
        }
        index++;
    }
    std::vector<bcos::bytes> vec;
    vec.assign(result_set.begin(), result_set.end());
    return vec;
}

std::string EcdhConnPSIClient::generateKey(int _mainProcess, int _subProcess)
{
    m_send_count++;
    std::string keys = "";
    if (_subProcess == -1)
    {
        // main process
        keys = "root:P2P-" + std::to_string(_mainProcess) + ":" +
               std::to_string(1 - (int)PartyType::Client) + "->" +
               std::to_string(1 - (int)PartyType::Server) + std::string{'\x01', '\x02'} +
               std::to_string(m_send_count);
    }
    else
    {
        keys = "root-" + std::to_string(_subProcess) + ":P2P-" + std::to_string(_mainProcess) +
               ":" + std::to_string(1 - (int)PartyType::Client) + "->" +
               std::to_string(1 - (int)PartyType::Server) + std::string{'\x01', '\x02'} +
               std::to_string(m_send_count);
    }
    return keys;
}

std::map<std::string, std::string> EcdhConnPSIClient::initHeader(const std::string& _taskId)
{
    std::map<std::string, std::string> header;
    header[SESSION_ID_HEAD] = _taskId;
    header[VERSION_HEAD] = "v1.0";
    header[TRACE_ID_HEAD] = "trace_" + _taskId;
    header[TOKEN_HEAD] = "token_" + _taskId;

    header[SOURCE_NODE_HEAD] = "HOST";
    header[TARGET_NODE_HEAD] = "GUEST";
    header[SOURCE_INST_HEAD] = "HOST";
    header[TARGET_INST_HEAD] = "GUEST";
    header[TECH_PROVIDER_CODE_HEAD] = "WEBANK";
    header[TOPIC_HEAD] = "host.0-guest.0";

    return header;
}