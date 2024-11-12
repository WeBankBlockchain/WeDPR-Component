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
 * @file EcdhConnPSIServer.cpp
 * @author: zachma
 * @date 2023-7-18
 */
#include "EcdhConnPSIServer.h"
#include "ppc-framework/protocol/GlobalConfig.h"
#include <bcos-utilities/DataConvertUtility.h>

using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace bcos;

EcdhConnPSIServer::EcdhConnPSIServer(
    EcdhConnPSIConfig::Ptr _config, EcdhConnTaskState::Ptr _taskState)
  : m_config(_config), m_taskState(_taskState)
{
    auto task = m_taskState->task();
    m_taskID = task->id();
    m_msgFactory = std::make_shared<PSIConnMessageFactory>();
}

void EcdhConnPSIServer::asyncStartRunTask(ppc::protocol::Task::ConstPtr _task)
{
    ECDH_CONN_LOG(INFO) << LOG_DESC("asyncStartRunTask: ") << printTaskInfo(_task);
    m_header = initHeader(_task->id());
}


void EcdhConnPSIServer::onHandShakeRequestHandler(const bcos::bytes& _msg)
{
    auto handRequestVo = m_msgFactory->parseHandshakeRequest(_msg);
    auto clientCurves = handRequestVo->GetCurve();
    auto clientHashList = handRequestVo->GetHash();
    auto supportedCurves = g_PPCConfig.supportedCurves(
        (uint8_t)TaskType::PSI, (uint8_t)TaskAlgorithmType::ECDH_PSI_2PC);
    auto supportedHashList = g_PPCConfig.supportedHashList(
        (uint8_t)TaskType::PSI, (uint8_t)TaskAlgorithmType::ECDH_PSI_2PC);
    std::set<int> localCurves(supportedCurves.begin(), supportedCurves.end());
    std::set<int> localHashTypes(supportedHashList.begin(), supportedHashList.end());
    bool curveSelected = false;
    bool hashTypeSelected = false;
    // sm-crypto enabled
    auto handShakeResponse = std::make_shared<HandShakeResponseVo>();
    if (g_PPCConfig.smCrypto())
    {
        // try to select SM2
        if (clientCurves.count((int32_t)ppc::protocol::ECCCurve::SM2) &&
            localCurves.count((int32_t)ppc::protocol::ECCCurve::SM2))
        {
            handShakeResponse->SetCurve((int32_t)ppc::protocol::ECCCurve::SM2);
            curveSelected = true;
        }
        // try to select SM3
        if (clientHashList.count((int32_t)ppc::protocol::HashImplName::SM3) &&
            localHashTypes.count((int32_t)ppc::protocol::HashImplName::SM3))
        {
            handShakeResponse->SetHash((int32_t)ppc::protocol::HashImplName::SM3);
            hashTypeSelected = true;
        }
    }

    if (!curveSelected)
    {
#ifdef ENABLE_CONN
        handShakeResponse->SetCurve((int32_t)ppc::protocol::ECCCurve::SM2);
#else
        handShakeResponse->SetCurve(selectCryptoAlgorithm(clientCurves, localCurves));
#endif
    }
    if (!hashTypeSelected)
    {
#ifdef ENABLE_CONN
        handShakeResponse->SetHash((int32_t)ppc::protocol::HashImplName::SHA256);
#else
        handShakeResponse->SetHash(selectCryptoAlgorithm(clientHashList, localHashTypes));
#endif
    }

    auto SelectedCurve = handShakeResponse->GetCurve();
    auto SelectedHashType = handShakeResponse->GetHash();
    if (SelectedCurve != int32_t(-1) && SelectedHashType != int32_t(-1))
    {
        auto cryptoBox =
            m_config->ecdhCryptoFactory()->createCryptoBox(SelectedCurve, SelectedHashType);
        m_taskState->setCryptoBox(cryptoBox);
        m_randomB = m_taskState->cryptoBox()->eccCrypto()->generateRandomScalar();
    }
    else
    {
        auto errorMsg = "onHandshakeRequest failed for no curve/hashType selected";
        ECDH_CONN_LOG(WARNING) << LOG_DESC(errorMsg) << LOG_KV("selectedCurve", SelectedCurve)
                               << LOG_KV("selectedHashType", SelectedHashType);
        auto result = std::make_shared<TaskResult>(m_taskID);
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandshakeFailed, errorMsg);
        result->setError(std::move(error));
        m_taskState->onTaskFinished(result, false);
        return;
    }

    // send message to response
    auto handshakeResponseBytes = m_msgFactory->createHandshakeResponse(handShakeResponse);
    auto requestBytes = m_msgFactory->createPSIConnMessageRequest(
        handshakeResponseBytes, generateKey((int)EcdhConnProcess::HandShakeProcess, -1));

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
                psi->computeAndEncryptSet(std::make_shared<bcos::bytes>(m_randomB));
            }
        },
        nullptr);
}
void EcdhConnPSIServer::onCipherClientToServer(const bcos::bytes& _msg)
{
    auto cipherExVo = m_msgFactory->parseCipherExchange(_msg);
    m_Q1_server = cipherExVo->cipherText();
    // 收到Q1 计算 Q2
    computeAndEncryptSetSecond(m_Q1_server, std::make_shared<bcos::bytes>(m_randomB));
}
void EcdhConnPSIServer::onSecondCipherClientToServer(const bcos::bytes& _msg)
{
    // 接收client发来的m_P2
    auto cipherExPb = m_msgFactory->parseCipherExchange(_msg);
    m_P2 = cipherExPb->cipherText();
    auto intersectionVec = tryIntersection(m_P2, m_Q2_server);
    bool synced = m_taskState->task()->syncResultToPeer();
    if (synced)
    {
        m_taskState->storePSIResult(m_config->dataResourceLoader(), intersectionVec);
    }
    m_taskState->onTaskFinished();
}

void EcdhConnPSIServer::computeAndEncryptSet(bcos::bytesPointer _randB)
{
    m_originInputs = m_taskState->loadAllData();
    if (!m_originInputs || m_originInputs->size() == 0)
    {
        ECDH_CONN_LOG(INFO) << LOG_DESC("Server computeAndEncryptSet")
                            << LOG_DESC("data size is empty!");
        auto result = std::make_shared<TaskResult>(m_taskState->task()->id());
        auto err = std::make_shared<bcos::Error>(-12222, "canceled by other peer");
        result->setError(std::move(err));
        m_taskState->onTaskFinished(result, false);
    }
    auto inputSize = m_originInputs->size();
    std::vector<bcos::bytes> _res_vectors;
    auto hash = m_taskState->cryptoBox()->hashImpl();
    for (int i = 0; i < inputSize; i++)
    {
        auto data = m_originInputs->getBytes(i);
        auto hashData = hash->hash(bcos::bytesConstRef(data.data(), data.size()));
        auto point = m_taskState->cryptoBox()->eccCrypto()->hashToCurve(hashData);
        auto hashSet = m_taskState->cryptoBox()->eccCrypto()->ecMultiply(point, *_randB);
        _res_vectors.push_back(hashSet);
    }

    m_P1 = _res_vectors;

    auto cipherBatchVo = std::make_shared<CipherBatchVo>("enc", 0, true);
    cipherBatchVo->setCount(m_P1.size());
    cipherBatchVo->setCipherText(m_P1);
    auto cipherBytes = m_msgFactory->createCipherExchange(cipherBatchVo);
    auto requestBytes = m_msgFactory->createPSIConnMessageRequest(
        cipherBytes, generateKey((int)EcdhConnProcess::CipherProcess, -1));

    auto message = m_config->ppcMsgFactory()->buildPPCMessage(m_taskState->task()->type(),
        m_taskState->task()->algorithm(), m_taskID, std::make_shared<bcos::bytes>(requestBytes));
    message->setSender(m_taskState->task()->selfParty()->id());
    message->setHeader(m_header);
    // shake hands with each other
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

void EcdhConnPSIServer::computeAndEncryptSetSecond(
    const std::vector<bcos::bytes>& _input, bcos::bytesPointer _randB)
{
    std::vector<bcos::bytes> _res_vectors;
    auto inputSize = _input.size();
    for (int i = 0; i < inputSize; i++)
    {
        auto inputData = _input.at(i);
        auto encryptSet = m_taskState->cryptoBox()->eccCrypto()->ecMultiply(inputData, *_randB);
        _res_vectors.push_back(encryptSet);
    }
    m_Q2_server = _res_vectors;

    auto cipherBatchVo = std::make_shared<CipherBatchVo>("dual.enc", 1, true);
    cipherBatchVo->setCount(m_Q2_server.size());
    cipherBatchVo->setCipherText(m_Q2_server);
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
                return;
            }
        },
        nullptr);
}

// input1 ∩ input2 ->
std::vector<bcos::bytes> EcdhConnPSIServer::tryIntersection(const std::vector<bcos::bytes>& _input1,
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

std::string EcdhConnPSIServer::generateKey(int _mainProcess, int _subProcess)
{
    m_send_count++;
    std::string keys = "";
    if (_subProcess == -1)
    {
        // main process
        keys = "root:P2P-" + std::to_string(_mainProcess) + ":" +
               std::to_string(1 - (int)PartyType::Server) + "->" +
               std::to_string(1 - (int)PartyType::Client) + std::string{'\x01', '\x02'} +
               std::to_string(m_send_count);
    }
    else
    {
        keys = "root-" + std::to_string(_subProcess) + ":P2P-" + std::to_string(_mainProcess) +
               ":" + std::to_string(1 - (int)PartyType::Server) + "->" +
               std::to_string(1 - (int)PartyType::Client) + std::string{'\x01', '\x02'} +
               std::to_string(m_send_count);
    }
    return keys;
}

int32_t EcdhConnPSIServer::selectCryptoAlgorithm(
    std::set<int32_t> _clientSupportedAlgorithms, std::set<int32_t> _localSupportedAlgorithms)
{
    for (auto const& algorithm : _clientSupportedAlgorithms)
    {
        if (_localSupportedAlgorithms.count(algorithm))
        {
            return algorithm;
        }
    }
    // Note: -1 means select algorithm failed
    return int32_t(-1);
}

std::map<std::string, std::string> EcdhConnPSIServer::initHeader(const std::string& _taskId)
{
    std::map<std::string, std::string> header;
    header[SESSION_ID_HEAD] = _taskId;
    header[VERSION_HEAD] = "v1.0";
    header[TRACE_ID_HEAD] = "trace_" + _taskId;
    header[TOKEN_HEAD] = "token_" + _taskId;

    header[SOURCE_NODE_HEAD] = "GUEST";
    header[TARGET_NODE_HEAD] = "HOST";
    header[SOURCE_INST_HEAD] = "GUEST";
    header[TARGET_INST_HEAD] = "HOST";
    header[TECH_PROVIDER_CODE_HEAD] = "WEBANK";
    header[TOPIC_HEAD] = "host.0-guest.0";

    return header;
}