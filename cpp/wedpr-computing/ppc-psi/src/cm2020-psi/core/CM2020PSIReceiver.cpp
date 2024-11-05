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
 * @file CM2020PSIReceiver.cpp
 * @author: shawnhe
 * @date 2022-12-7
 */

#include "CM2020PSIReceiver.h"
#include "CM2020PSI.h"
#include "openssl/rand.h"
#include "ppc-crypto/src/prng/AESPRNG.h"
#include "ppc-tools/src/common/TransTools.h"
#include "wedpr-protocol/tars/TarsSerialize.h"
#include <tbb/parallel_for.h>

using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace ppc::tools;

CM2020PSIReceiver::CM2020PSIReceiver(
    CM2020PSIConfig::Ptr _config, TaskState::Ptr _taskState, SimplestOT::Ptr _ot)
  : m_config(std::move(_config)), m_taskState(std::move(_taskState)), m_ot(std::move(_ot))
{
    auto task = m_taskState->task();
    m_taskID = task->id();
    m_params = std::make_shared<TaskParams>(task->param());
    m_params->setSyncResults(task->syncResultToPeer());
    m_params->setLowBandwidth(task->lowBandwidth());
    m_cm2020Result = std::make_shared<CM2020PSIResult>(m_taskID);
    m_startTimePoint = std::chrono::high_resolution_clock::now();
    m_costs.resize(4);
    m_progress = std::make_shared<Progress>(m_config->threadPool());
}

void CM2020PSIReceiver::asyncRunTask()
{
    CM2020_PSI_LOG(INFO) << LOG_DESC("asyncRunTask as receiver") << LOG_KV("taskID", m_taskID);

    m_progress->reset(3, [self = weak_from_this()]() {
        /**
         * 1. prepare inputs done
         * 2. run random OT done
         * 3. sender size received
         */
        auto receiver = self.lock();
        if (!receiver)
        {
            return;
        }
        receiver->preprocessInputs();
        receiver->doOprf();
    });

    m_config->threadPool()->enqueue([self = weak_from_this()]() {
        auto receiver = self.lock();
        if (!receiver)
        {
            return;
        }
        receiver->runReceiver();
    });
}

void CM2020PSIReceiver::runReceiver()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("runReceiver") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("syncResults", m_params->enableSyncResults())
                         << LOG_KV("bucketNumber", m_params->bucketNumber());
    try
    {
        ppctars::CM2020Params cm2020Params;
        cm2020Params.bucketNumber = m_params->bucketNumber();
        cm2020Params.enableSyncResults = m_params->enableSyncResults();
        cm2020Params.lowBandwidth = m_params->lowBandwidth();
        cm2020Params.seed.resize(16);
        if (RAND_bytes(cm2020Params.seed.data(), 16) != 1)
        {
            BOOST_THROW_EXCEPTION(CM2020Exception());
        }

        auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
            uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC), m_taskID,
            std::make_shared<bcos::bytes>());
        message->setMessageType(uint8_t(CM2020PSIMessageType::HELLO_SENDER));
        ppctars::serialize::encode(cm2020Params, *message->data());
        CM2020_PSI_LOG(INFO) << LOG_BADGE("runReceiver") << LOG_KV("taskID", m_taskID)
                             << LOG_KV("syncResults", m_params->enableSyncResults())
                             << LOG_KV("message data:", *(bcos::toHexString(*(message->data()))))
                             << LOG_KV("message size:", message->data()->size())
                             << LOG_KV("bucketNumber", m_params->bucketNumber());
        // shake hands with each other
        m_config->front()->asyncSendMessage(
            m_taskState->peerID(), message, m_config->networkTimeout(),
            [self = weak_from_this()](bcos::Error::Ptr _error) {
                auto receiver = self.lock();
                if (!receiver)
                {
                    return;
                }
                if (_error && _error->errorCode())
                {
                    receiver->onReceiverTaskDone(std::move(_error));
                }
            },
            nullptr);

        prepareInputs();
    }
    catch (const std::exception& e)
    {
        onReceiverException("runReceiver", e);
    }
}


void CM2020PSIReceiver::onHandshakeDone()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("onHandshakeDone") << LOG_KV("taskID", m_taskID);
    try
    {
        m_startTimePoint = std::chrono::high_resolution_clock::now();
        runRandomOT();
    }
    catch (const std::exception& e)
    {
        onReceiverException("runRandomOT", e);
    }
}


void CM2020PSIReceiver::prepareInputs()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("prepareInputs") << LOG_KV("taskID", m_taskID);
    try
    {
        auto originData = m_taskState->task()->selfParty()->dataResource()->rawData();
        if (!originData.empty())
        {
            m_ioByInterface = true;
        }

        m_originInputs = m_taskState->loadAllData();
        if (!m_originInputs || m_originInputs->size() == 0)
        {
            BOOST_THROW_EXCEPTION(CM2020Exception() << bcos::errinfo_comment("data is empty"));
        }

        m_rInputSize = m_originInputs->size();
        m_originLocations.reserve(m_rInputSize);
        m_originLocations.resize(m_rInputSize);
        m_oprfOutputs.reserve(m_rInputSize);
        m_oprfOutputs.resize(m_rInputSize);

        syncInputsSize();
    }
    catch (const std::exception& e)
    {
        onReceiverException("prepareInputs", e);
    }
}

void CM2020PSIReceiver::syncInputsSize()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("syncInputsSize") << LOG_KV("taskID", m_taskID);

    auto data = std::make_shared<bcos::bytes>();
    encodeUnsignedNum(data, m_rInputSize);
    CM2020_PSI_LOG(TRACE) << LOG_BADGE("syncInputsSize") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("data", *bcos::toHexString(*data))
                          << LOG_KV("m_rInputSize", m_rInputSize);
    auto message = m_config->ppcMsgFactory()->buildPPCMessage(
        uint8_t(TaskType::PSI), uint8_t(TaskAlgorithmType::CM_PSI_2PC), m_taskID, data);
    message->setMessageType(uint8_t(CM2020PSIMessageType::RECEIVER_SIZE));

    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](bcos::Error::Ptr _error) {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                receiver->onReceiverTaskDone(std::move(_error));
            }
        },
        nullptr);

    m_progress->mark<std::string>("PREPARE_INPUTS");
}

void CM2020PSIReceiver::runRandomOT()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("runOT as sender") << LOG_KV("taskID", m_taskID);
    m_otState = m_ot->senderGeneratePointA();

    auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
        uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC), m_taskID, m_otState.second);
    message->setMessageType(uint8_t(CM2020PSIMessageType::POINT_A));

    // send point A to ot receiver
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](bcos::Error::Ptr _error) {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                receiver->onReceiverTaskDone(std::move(_error));
            }
        },
        nullptr);
}

void CM2020PSIReceiver::onBatchPointBReceived(PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("handleBatchPointB") << LOG_KV("taskID", m_taskID);

    try
    {
        m_senderKeys = m_ot->finishSender(m_otState.first, m_otState.second, _message->data());
        m_progress->mark<std::string>("RANDOM_OT");
    }
    catch (const std::exception& e)
    {
        onReceiverException("onBatchPointBReceived", e);
    }
}

void CM2020PSIReceiver::onSenderSizeReceived(front::PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("onSenderSizeReceived") << LOG_KV("taskID", m_taskID);
    try
    {
        decodeUnsignedNum(m_sInputSize, _message->data());
        m_progress->mark<std::string>("RECEIVE_SIZE");
    }
    catch (const std::exception& e)
    {
        onReceiverException("onSenderSizeReceived", e);
    }
}


void CM2020PSIReceiver::preprocessInputs()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    try
    {
        CM2020_PSI_LOG(INFO) << LOG_BADGE("preprocessInputs") << LOG_KV("taskID", m_taskID);
        bcos::bytesConstRef seed(m_taskID);
        auto hash = m_config->hash();

        tbb::parallel_for(tbb::blocked_range<size_t>(0U, m_rInputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto state = hash->init();
                hash->update(state, seed);
                bcos::bytes data = m_originInputs->getBytes(i);
                hash->update(state, bcos::ref(bcos::bytes(data.begin(), data.end())));
                auto res = hash->final(state);
                memcpy((bcos::byte*)m_originLocations[i].data(), res.data(), 8 * sizeof(uint32_t));
            }
        });
    }
    catch (const std::exception& e)
    {
        onReceiverException("preprocessInputs", e);
    }
}

// considering the memory occupation, we handle the global matrix by 'm_handleWidth' column per
// round, when and only when this round ends, the next round will be made
void CM2020PSIReceiver::doOprf()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    try
    {
        if (m_oprfRound == 0)
        {
            auto end = std::chrono::high_resolution_clock::now();
            // costs of rot and first three steps
            m_costs[0] =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - m_startTimePoint)
                    .count();

            initMatrixParams();
        }

        CM2020_PSI_LOG(INFO) << LOG_BADGE("doOprf") << LOG_KV("taskID", m_taskID)
                             << LOG_KV("round", m_oprfRound);

        uint32_t offset = m_oprfRound * m_handleWidth;
        if (offset >= m_params->bucketNumber())
        {
            auto end = std::chrono::high_resolution_clock::now();
            // costs of oprf
            m_costs[1] =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - m_startTimePoint)
                    .count();

            // after negotiating matrix and computing oprf,
            // we can receive hash from sender and compute intersection
            m_config->threadPool()->enqueue([self = weak_from_this()]() {
                auto receiver = self.lock();
                if (!receiver)
                {
                    return;
                }
                receiver->doPsi();
            });
        }
        else
        {
            m_currentWidth = offset + m_handleWidth < m_params->bucketNumber() ?
                                 m_handleWidth :
                                 m_params->bucketNumber() - offset;

            m_progress->reset(m_currentWidth + 1 + 1, [self = weak_from_this()]() {
                auto receiver = self.lock();
                if (!receiver)
                {
                    return;
                }
                receiver->increaseOprfRound();
                receiver->doOprf();
            });

            constructMatrices(offset, m_currentWidth);

            for (uint32_t i = 0; i < m_currentWidth; ++i)
            {
                m_config->threadPool()->enqueue([self = weak_from_this(), i, offset]() {
                    auto receiver = self.lock();
                    if (!receiver)
                    {
                        return;
                    }

                    // encrypt matrix A ^ matrix Delta by column, and send it to sender
                    receiver->negotiateMatrix(offset + i, i);
                    receiver->progress()->mark<std::string>("MATRIX" + std::to_string(i));
                });
            }

            // when we get matrix A, we can compute the inputs for final hash and negotiate matrix
            // with counterparty in parallel
            computeOprfOutputs(offset, m_currentWidth);
            m_progress->mark<std::string>("OPRF");
        }
    }
    catch (const std::exception& e)
    {
        onReceiverException("doOprf", e);
    }
}

void CM2020PSIReceiver::initMatrixParams()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    uint32_t maxInputSize = std::max(m_rInputSize, m_sInputSize);

    m_handleWidth = m_params->bucketNumber();

    if (maxInputSize > (1 << DEFAULT_HANDLE_WIDTH_POWER))
    {
        // while handling mass data, we do oprf block by block to prevent OOM
        m_handleWidth = m_params->bucketNumber() /
                        (1 << int(std::ceil(log2(maxInputSize) - DEFAULT_HANDLE_WIDTH_POWER)));
        m_handleWidth = std::max(m_handleWidth, (uint32_t)MIN_HANDLE_WIDTH);
    }

    m_bucketSizeInBytes = std::max(
        uint32_t(MIN_BUCKET_SIZE / 8), uint32_t(((long double)maxInputSize * ENCODE_RATE + 7) / 8));
    m_mask = m_bucketSizeInBytes * 8;

    CM2020_PSI_LOG(INFO) << LOG_BADGE("initMatrixParams") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("rInputSize", m_rInputSize)
                         << LOG_KV("bucketNumber", m_params->bucketNumber())
                         << LOG_KV("handleWidth", m_handleWidth)
                         << LOG_KV("bucketSizeInBytes", m_bucketSizeInBytes)
                         << LOG_KV("mask", m_mask);
}

void CM2020PSIReceiver::constructMatrices(uint32_t _offset, uint32_t _width)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("constructMatrices") << LOG_KV("offset", _offset)
                         << LOG_KV("width", _width) << LOG_KV("taskID", m_taskID);

    if (_offset == 0)
    {
        m_matrixA.reserve(m_handleWidth);
        m_matrixA.resize(m_handleWidth);

        m_matrixDelta.reserve(m_handleWidth);
        m_matrixDelta.resize(m_handleWidth);
    }

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, _width), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            // construct matrix A
            if (_offset == 0)
            {
                m_matrixA[i].reserve(m_bucketSizeInBytes);
                m_matrixA[i].resize(m_bucketSizeInBytes);
                m_matrixDelta[i].reserve(m_bucketSizeInBytes);
                m_matrixDelta[i].resize(m_bucketSizeInBytes);
            }
            AESPRNG prng(m_senderKeys[_offset + i][0]);
            prng.generate(m_matrixA[i].data(), m_bucketSizeInBytes);

            // int matrix D
            memset(m_matrixDelta[i].data(), 255, m_bucketSizeInBytes);
        }
    });
}


void CM2020PSIReceiver::negotiateMatrix(uint32_t _bucketIndex, uint32_t _matrixIndex)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(DEBUG) << LOG_BADGE("negotiateMatrix") << LOG_KV("bucketIndex", _bucketIndex)
                          << LOG_KV("taskID", m_taskID);

    for (uint32_t j = 0; j < m_rInputSize; j++)
    {
        uint32_t location = (m_originLocations[j][(_bucketIndex) % 4] * (_bucketIndex) +
                                m_originLocations[j][4 + (_bucketIndex) % 4]) %
                            m_mask;
        m_matrixDelta[_matrixIndex][location >> 3] &= ~(1 << (location & 7));
    }

    auto buffer = std::make_shared<bcos::bytes>(m_bucketSizeInBytes);

    // use key[1] to encrypt matrixA ^ matrixDelta
    AESPRNG prng(m_senderKeys[_bucketIndex][1]);
    prng.generate(buffer->data(), m_bucketSizeInBytes);

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, m_bucketSizeInBytes), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            buffer->at(i) ^= m_matrixA[_matrixIndex][i];
            buffer->at(i) ^= m_matrixDelta[_matrixIndex][i];
        }
    });

    // handle m_bucketSizeInBytes > 4M
    uint32_t totalRound =
        (m_bucketSizeInBytes + MAX_SEND_BUFFER_LENGTH - 1) / MAX_SEND_BUFFER_LENGTH;
    for (uint32_t round = 0; round < totalRound; round++)
    {
        uint32_t rLen = round * MAX_SEND_BUFFER_LENGTH;
        uint32_t currentLen = uint32_t((round + 1) * MAX_SEND_BUFFER_LENGTH) > m_bucketSizeInBytes ?
                                  m_bucketSizeInBytes - rLen :
                                  MAX_SEND_BUFFER_LENGTH;

        auto sendBuffer = std::make_shared<bcos::bytes>(
            buffer->begin() + rLen, buffer->begin() + rLen + currentLen);
        auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
            uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC), m_taskID, sendBuffer);
        message->setSeq(_bucketIndex * totalRound + round);
        message->setMessageType(uint8_t(CM2020PSIMessageType::MATRIX));

        if (m_params->lowBandwidth())
        {
            auto error = m_config->sendMessage(m_taskState->peerID(), message);
            if (error && error->errorCode())
            {
                onReceiverTaskDone(std::move(error));
            }
        }
        else
        {
            m_config->front()->asyncSendMessage(
                m_taskState->peerID(), message, m_config->networkTimeout(),
                [self = weak_from_this()](bcos::Error::Ptr _error) {
                    auto receiver = self.lock();
                    if (!receiver)
                    {
                        return;
                    }
                    if (_error && _error->errorCode())
                    {
                        receiver->onReceiverTaskDone(std::move(_error));
                    }
                },
                nullptr);
        }
    }
}

void CM2020PSIReceiver::computeOprfOutputs(uint32_t _offset, uint32_t _width)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("computeOprfOutputs") << LOG_KV("offset", _offset)
                         << LOG_KV("width", _width) << LOG_KV("taskID", m_taskID);
    try
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, m_rInputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto newSize = (_offset + _width + 7) / 8;
                m_oprfOutputs[i].reserve(newSize);
                m_oprfOutputs[i].resize(newSize);
                for (uint32_t j = 0; j < _width; j++)
                {
                    uint32_t location = (m_originLocations[i][(_offset + j) % 4] * (_offset + j) +
                                            m_originLocations[i][4 + (_offset + j) % 4]) %
                                        m_mask;

                    m_oprfOutputs[i][(_offset + j) >> 3] |=
                        (uint8_t)((bool)(m_matrixA[j][location >> 3] & (1 << (location & 7))))
                        << ((_offset + j) & 7);
                }
            }
        });
    }
    catch (const std::exception& e)
    {
        onReceiverException("computeOprfOutputs", e);
    }
}

void CM2020PSIReceiver::onDoNextRoundReceived()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("onDoNextRoundReceived") << LOG_KV("taskID", m_taskID);
    m_progress->mark<std::string>("NEXT");
}

void CM2020PSIReceiver::doPsi()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("doPsi") << LOG_KV("taskID", m_taskID);

    try
    {
        // receive 2^18 hashes per round
        uint32_t number = MAX_SEND_BUFFER_LENGTH / RESULT_LEN_BYTE;
        m_maxHashSeq = (m_sInputSize + number - 1) / number;

        m_progress->reset(m_maxHashSeq, [self = weak_from_this()]() {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            receiver->finishPsi();
        });

        clearOprfMemory();
        computeHash();

        m_rResults = std::make_shared<std::vector<uint8_t>>(m_rInputSize, 0);
        if (m_params->enableSyncResults())
        {
            m_sResults = std::make_shared<std::vector<uint8_t>>(m_sInputSize, 0);
        }

        bcos::WriteGuard l(x_receivedHash);
        m_hashReady.exchange(true);

        tryComputeIntersection();
    }
    catch (const std::exception& e)
    {
        onReceiverException("doPsi", e);
    }
}

void CM2020PSIReceiver::clearOprfMemory()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("clearOprfMemory") << LOG_KV("taskID", m_taskID);

    std::vector<std::array<uint32_t, 8>>().swap(m_originLocations);
    std::vector<bcos::bytes>().swap(m_matrixA);
    std::vector<bcos::bytes>().swap(m_matrixDelta);
    MallocExtension::instance()->ReleaseFreeMemory();
}

void CM2020PSIReceiver::computeHash()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("computeHash") << LOG_KV("taskID", m_taskID);

    // avoid OOM
    uint32_t totalRound = (m_rInputSize + MAX_COMPUTE_HASH_SIZE - 1) / MAX_COMPUTE_HASH_SIZE;

    auto hash = m_config->hash();
    for (uint32_t round = 0; round < totalRound; round++)
    {
        uint32_t offset = round * MAX_COMPUTE_HASH_SIZE;
        uint32_t currentNum = (round + 1) * MAX_COMPUTE_HASH_SIZE > m_rInputSize ?
                                  m_rInputSize - offset :
                                  MAX_COMPUTE_HASH_SIZE;

        std::vector<u128> hashes;
        hashes.reserve(currentNum);
        hashes.resize(currentNum);

        tbb::parallel_for(tbb::blocked_range<size_t>(0U, currentNum), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto res = hash->hash(bcos::ref(m_oprfOutputs[offset + i]));
                memcpy((bcos::byte*)&hashes[i], res.data(), RESULT_LEN_BYTE);
            }
        });

        for (uint32_t i = 0; i < currentNum; i++)
        {
            m_hashes[hashes[i]] = offset + i;
        }
    }

    // clear oprf outputs
    std::vector<std::vector<bcos::byte>>().swap(m_oprfOutputs);
    MallocExtension::instance()->ReleaseFreeMemory();
}


void CM2020PSIReceiver::tryComputeIntersection()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("tryComputeIntersection") << LOG_KV("taskID", m_taskID);
    for (auto& it : m_receivedHash)
    {
        uint32_t round = it.first;
        bcos::bytesPointer buffer = it.second;
        m_config->threadPool()->enqueue([round, buffer, self = weak_from_this()]() {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            receiver->computeIntersection(round, buffer);
        });
    }
}

void CM2020PSIReceiver::onHashesReceived(PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(DEBUG) << LOG_BADGE("handleReceivedHash") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("seq", _message->seq());

    try
    {
        {
            bcos::WriteGuard l(x_receivedHash);
            if (!m_hashReady)
            {
                m_receivedHash[_message->seq()] = _message->data();
                return;
            }
        }
        computeIntersection(_message->seq(), _message->data());
    }
    catch (const std::exception& e)
    {
        onReceiverException("onHashesReceived", e);
    }
}

void CM2020PSIReceiver::computeIntersection(uint32_t _round, bcos::bytesPointer _buffer)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(DEBUG) << LOG_BADGE("computeIntersection") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("seq", _round);
    try
    {
        uint32_t hashNum = _buffer->size() / RESULT_LEN_BYTE;
        uint32_t handleNum = MAX_SEND_BUFFER_LENGTH / RESULT_LEN_BYTE;
        for (uint32_t i = 0; i < hashNum; i++)
        {
            u128 key = *(u128*)(_buffer->data() + i * RESULT_LEN_BYTE);
            if (m_hashes.find(key) != m_hashes.end())
            {
                m_rResults->at(m_hashes[key]) = 1;
                if (m_params->enableSyncResults())
                {
                    m_sResults->at(_round * handleNum + i) = 1;
                }
                m_resultCount++;
            }
        }

        m_progress->mark<uint64_t>((uint64_t)_round);
    }
    catch (const std::exception& e)
    {
        onReceiverException("computeIntersection", e);
    }
}

void CM2020PSIReceiver::finishPsi()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    try
    {
        auto end = std::chrono::high_resolution_clock::now();
        // costs of psi
        m_costs[2] =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - m_startTimePoint).count();

        CM2020_PSI_LOG(INFO) << LOG_BADGE("finishPsi") LOG_KV("taskID", m_taskID);
        if (m_params->enableSyncResults())
        {
            syncResults(m_resultCount);
        }

        if (m_resultCount > 0)
        {
            // save results
            saveResults();
        }

        onReceiverTaskDone(nullptr);
    }
    catch (const std::exception& e)
    {
        onReceiverException("finishPsi", e);
    }
}

void CM2020PSIReceiver::syncResults(uint32_t _count)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("syncResults") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("count", _count);

    // send the count of results
    auto countData = std::make_shared<bcos::bytes>();
    encodeUnsignedNum(countData, uint32_t(_count));
    auto message = m_config->ppcMsgFactory()->buildPPCMessage(
        uint8_t(TaskType::PSI), uint8_t(TaskAlgorithmType::CM_PSI_2PC), m_taskID, countData);
    message->setMessageType(uint8_t(CM2020PSIMessageType::RESULTS_SIZE));

    auto error = m_config->sendMessage(m_taskState->peerID(), message);
    if (error && error->errorCode())
    {
        onReceiverTaskDone(std::move(error));
    }

    if (_count == 0)
    {
        return;
    }

    // send 2^20 indexes per round
    uint32_t number = MAX_SEND_BUFFER_LENGTH / sizeof(uint32_t);
    uint32_t count = 0, round = 0, totalCount = 0;
    uint32_t currentLen = (round + 1) * number > _count ? _count - round * number : number;
    auto buffer = std::make_shared<bcos::bytes>(currentLen * sizeof(uint32_t));

    for (uint32_t index = 0; index < m_sInputSize; index++)
    {
        if (m_sResults->at(index))
        {
            encodeUnsignedNum(buffer->data() + count * sizeof(uint32_t), index);
            count++, totalCount++;
            if (count == currentLen)
            {
                auto resMessage = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(TaskType::PSI),
                    uint8_t(TaskAlgorithmType::CM_PSI_2PC), m_taskID, buffer);
                resMessage->setMessageType(uint8_t(CM2020PSIMessageType::RESULTS));
                resMessage->setSeq(round);

                if (m_params->lowBandwidth())
                {
                    error = m_config->sendMessage(m_taskState->peerID(), resMessage);
                    if (error && error->errorCode())
                    {
                        onReceiverTaskDone(std::move(error));
                    }
                }
                else
                {
                    m_config->front()->asyncSendMessage(
                        m_taskState->peerID(), resMessage, m_config->networkTimeout(),
                        [self = weak_from_this()](bcos::Error::Ptr _error) {
                            auto receiver = self.lock();
                            if (!receiver)
                            {
                                return;
                            }
                            if (_error && _error->errorCode())
                            {
                                receiver->onReceiverTaskDone(std::move(_error));
                            }
                        },
                        nullptr);
                }

                if (totalCount < _count)
                {
                    count = 0, round++;
                    currentLen = (round + 1) * number > _count ? _count - round * number : number;
                    buffer = std::make_shared<bcos::bytes>(currentLen * sizeof(uint32_t));
                }
            }
        }
    }
}

void CM2020PSIReceiver::saveResults()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("saveResults") << LOG_KV("taskID", m_taskID);
    try
    {
        if (m_ioByInterface)
        {
            std::vector<std::string> results;
            for (uint32_t index = 0; index < m_rInputSize; index++)
            {
                if (m_rResults->at(index))
                {
                    auto line = m_originInputs->getBytes(index);
                    results.emplace_back(std::string(line.begin(), line.end()));
                }
            }
            m_cm2020Result->m_outputs.emplace_back(std::move(results));
        }
        else
        {
            DataBatch::Ptr finalResults = std::make_shared<DataBatch>();
            for (uint32_t index = 0; index < m_rInputSize; index++)
            {
                if (m_rResults->at(index))
                {
                    finalResults->append<bcos::bytes>(m_originInputs->getBytes(index));
                }
            }
            CM2020_PSI_LOG(INFO) << LOG_BADGE("before dedup") << LOG_KV("taskID", m_taskID)
                                 << LOG_KV("originCount", finalResults->size());
            m_resultCount = dedupDataBatch(finalResults);
            CM2020_PSI_LOG(INFO) << LOG_BADGE("after dedup") << LOG_KV("taskID", m_taskID)
                                 << LOG_KV("resultCount", m_resultCount);
            m_taskState->writeLines(finalResults, DataSchema::Bytes);
        }
    }
    catch (const std::exception& e)
    {
        onReceiverException("saveResults", e);
    }
}

void CM2020PSIReceiver::onReceiverException(const std::string& _module, const std::exception& e)
{
    CM2020_PSI_LOG(ERROR) << LOG_BADGE(_module) LOG_KV("taskID", m_taskID)
                          << LOG_KV("exception", boost::diagnostic_information(e));
    auto error = std::make_shared<bcos::Error>(
        (int)CM2020PSIRetCode::ON_EXCEPTION, boost::diagnostic_information(e));
    onReceiverTaskDone(error);
}

void CM2020PSIReceiver::onReceiverTaskDone(bcos::Error::Ptr _error)
{
    if (m_taskState->taskDone() && (!_error || _error->errorCode() == 0))
    {
        return;
    }

    CM2020_PSI_LOG(INFO) << LOG_BADGE("onReceiverTaskDone") LOG_KV("taskID", m_taskID);

    std::string message;
    if (_error)
    {
        message = "\nStatus: FAIL\nMessage: " + _error->errorMessage();
        m_cm2020Result->setError(std::move(_error));
    }
    else
    {
        auto end = std::chrono::high_resolution_clock::now();
        // costs of aftermath
        m_costs[3] =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - m_startTimePoint).count();

        uint64_t communication =
            (uint64_t)m_bucketSizeInBytes * (uint64_t)m_params->bucketNumber() +
            (uint64_t)m_sInputSize * (uint64_t)sizeof(u128);

        if (m_params->enableSyncResults())
        {
            communication += (uint64_t)m_resultCount * (uint64_t)sizeof(uint32_t);
        }

        communication /= (uint64_t)(1024 * 1024);

        m_cm2020Result->m_enableSyncResults = m_params->enableSyncResults();
        m_cm2020Result->m_bucketNumber = m_params->bucketNumber();
        m_cm2020Result->m_communication = std::to_string(communication) + "MB";
        m_cm2020Result->m_intersections = m_resultCount;
        m_cm2020Result->m_party0Size = m_rInputSize;
        m_cm2020Result->m_party1Size = m_sInputSize;

        message = "\nStatus: SUCCESS\nOrigin Inputs Number: " + std::to_string(m_rInputSize) +
                  "\nIntersections Number: " + std::to_string(m_resultCount) +
                  "\nCommunication: " + std::to_string(communication) +
                  "MB\nTotal Costs: " + std::to_string(m_costs[3]) + "ms\n";
    }
    m_taskState->onTaskFinished(m_cm2020Result, true);

    CM2020_PSI_LOG(INFO) << LOG_BADGE("receiverTaskDone") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("detail", message);
}
