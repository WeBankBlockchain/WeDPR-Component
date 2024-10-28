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
 * @file CM2020PSISender.cpp
 * @author: shawnhe
 * @date 2022-12-7
 */

#include "CM2020PSISender.h"
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

CM2020PSISender::CM2020PSISender(CM2020PSIConfig::Ptr _config, TaskState::Ptr _taskState,
    std::shared_ptr<boost::asio::io_service> _ioService, SimplestOT::Ptr _ot,
    std::shared_ptr<bcos::ThreadPool> _threadPool)
  : m_config(std::move(_config)), m_taskState(std::move(_taskState)), m_ot(std::move(_ot))
{
    auto task = m_taskState->task();
    m_taskID = task->id();
    m_params = std::make_shared<TaskParams>(task->param());
    m_params->setSyncResults(task->syncResultToPeer());
    m_cm2020Result = std::make_shared<CM2020PSIResult>(m_taskID);
    m_channelManager = std::make_shared<front::PPCChannelManager>(std::move(_ioService), nullptr);
    m_channelManager->setThreadPool(std::move(_threadPool));
    m_channel = m_channelManager->buildChannelForTask(m_taskID);
    m_startTimePoint = std::chrono::high_resolution_clock::now();
    m_costs.resize(4);
    m_matrixProgress = std::make_shared<Progress>(m_config->threadPool());
    m_progress = std::make_shared<Progress>(m_config->threadPool());
}


void CM2020PSISender::asyncRunTask()
{
    CM2020_PSI_LOG(INFO) << LOG_DESC("asyncRunTask as sender") << LOG_KV("taskID", m_taskID);

    m_progress->reset(4, [self = weak_from_this()]() {
        /**
         * 1. prepare inputs done
         * 2. run random OT done
         * 3. receiver size received
         * 4. handshake done
         */
        auto sender = self.lock();
        if (!sender)
        {
            return;
        }
        sender->preprocessInputs();
        sender->doOprf();
    });

    m_config->threadPool()->enqueue([self = weak_from_this()]() {
        auto sender = self.lock();
        if (!sender)
        {
            return;
        }
        sender->runSender();
    });
}

void CM2020PSISender::runSender()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("runSender") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("syncResults", m_params->enableSyncResults())
                         << LOG_KV("bucketNumber", m_params->bucketNumber());

    try
    {
        auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
            uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC), m_taskID,
            std::make_shared<bcos::bytes>());
        message->setMessageType(uint8_t(CM2020PSIMessageType::HELLO_RECEIVER));

        // shake hands with each other
        m_config->front()->asyncSendMessage(
            m_taskState->peerID(), message, m_config->networkTimeout(),
            [self = weak_from_this()](bcos::Error::Ptr _error) {
                auto sender = self.lock();
                if (!sender)
                {
                    return;
                }
                if (_error && _error->errorCode())
                {
                    sender->onSenderTaskDone(std::move(_error));
                }
            },
            nullptr);

        prepareInputs();
    }
    catch (const std::exception& e)
    {
        onSenderException("runSender", e);
    }
}

void CM2020PSISender::onHandshakeDone(front::PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("onHandshakeDone") << LOG_KV("taskID", m_taskID);
    m_startTimePoint = std::chrono::high_resolution_clock::now();
    ppctars::CM2020Params tarsParams;
    ppctars::serialize::decode(*_message->data(), tarsParams);
    m_params->setSyncResults(tarsParams.enableSyncResults);
    m_params->setBucketNumber(uint16_t(tarsParams.bucketNumber));
    m_params->setSeed(tarsParams.seed);
    m_params->setLowBandwidth(tarsParams.lowBandwidth);

    m_progress->mark<std::string>("HANDSHAKE");
}

void CM2020PSISender::prepareInputs()
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

        m_sInputSize = m_originInputs->size();
        m_originLocations.reserve(m_sInputSize);
        m_originLocations.resize(m_sInputSize);
        m_oprfOutputs.reserve(m_sInputSize);
        m_oprfOutputs.resize(m_sInputSize);

        syncInputsSize();
    }
    catch (const std::exception& e)
    {
        onSenderException("prepareInputs", e);
    }
}

void CM2020PSISender::syncInputsSize()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("syncInputsSize") << LOG_KV("taskID", m_taskID);

    auto data = std::make_shared<bcos::bytes>();
    encodeUnsignedNum(data, m_sInputSize);
    auto message = m_config->ppcMsgFactory()->buildPPCMessage(
        uint8_t(TaskType::PSI), uint8_t(TaskAlgorithmType::CM_PSI_2PC), m_taskID, data);
    message->setMessageType(uint8_t(CM2020PSIMessageType::SENDER_SIZE));

    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](bcos::Error::Ptr _error) {
            auto sender = self.lock();
            if (!sender)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                sender->onSenderTaskDone(std::move(_error));
            }
        },
        nullptr);

    m_progress->mark<std::string>("PREPARE_INPUTS");
}

void CM2020PSISender::onReceiverSizeReceived(front::PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }

    CM2020_PSI_LOG(INFO) << LOG_BADGE("onReceiverSizeReceived") << LOG_KV("taskID", m_taskID);
    try
    {
        decodeUnsignedNum(m_rInputSize, _message->data());
        m_progress->mark<std::string>("RECEIVE_SIZE");
    }
    catch (const std::exception& e)
    {
        onSenderException("onReceiverSizeReceived", e);
    }
}

void CM2020PSISender::onPointAReceived(front::PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("onPointAReceived") << LOG_KV("taskID", m_taskID);
    try
    {
        auto pointA = _message->data();

        bcos::bytes seed(16);
        if (RAND_bytes(seed.data(), 16) != 1)
        {
            BOOST_THROW_EXCEPTION(CM2020Exception());
        }
        AESPRNG::Ptr prng = std::make_shared<AESPRNG>(seed);
        m_otChoices = std::make_shared<BitVector>();
        m_otChoices->randomize(prng, (m_params->bucketNumber() + 7) / 8);

        auto retPair = m_ot->receiverGeneratePointsB(m_otChoices, pointA);

        // send batch point B to ot sender
        auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(TaskType::PSI),
            uint8_t(TaskAlgorithmType::CM_PSI_2PC), m_taskID, retPair.first);
        message->setMessageType(uint8_t(CM2020PSIMessageType::POINT_B_ARRAY));

        m_config->front()->asyncSendMessage(
            m_taskState->peerID(), message, m_config->networkTimeout(),
            [self = weak_from_this()](bcos::Error::Ptr _error) {
                auto sender = self.lock();
                if (!sender)
                {
                    return;
                }
                if (_error && _error->errorCode())
                {
                    sender->onSenderTaskDone(std::move(_error));
                }
            },
            nullptr);

        // finish ot receiver
        m_receiverKeys = m_ot->finishReceiver(pointA, retPair.second);
        m_progress->mark<std::string>("RANDOM_OT");
    }
    catch (const std::exception& e)
    {
        onSenderException("onPointAReceived", e);
    }
}

void CM2020PSISender::preprocessInputs()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("preprocessInputs") << LOG_KV("taskID", m_taskID);
    try
    {
        bcos::bytesConstRef seed(m_taskID);
        auto hash = m_config->hash();
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, m_sInputSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto state = hash->init();
                hash->update(state, seed);
                auto data = m_originInputs->getBytes(i);
                hash->update(state, bcos::ref(bcos::bytes(data.begin(), data.end())));
                auto res = hash->final(state);
                memcpy((bcos::byte*)m_originLocations[i].data(), res.data(), 8 * sizeof(uint32_t));
            }
        });
    }
    catch (const std::exception& e)
    {
        onSenderException("preprocessInputs", e);
    }
}

void CM2020PSISender::doOprf()
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
            // costs of rot and prepare inputs
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
                auto sender = self.lock();
                if (!sender)
                {
                    return;
                }
                sender->doPsi();
            });
        }
        else
        {
            uint32_t currentWidth = offset + m_handleWidth < m_params->bucketNumber() ?
                                        m_handleWidth :
                                        m_params->bucketNumber() - offset;
            uint32_t totalRound =
                (m_bucketSizeInBytes + MAX_SEND_BUFFER_LENGTH - 1) / MAX_SEND_BUFFER_LENGTH;

            constructMatrices(offset, currentWidth);
            m_progress->reset(
                currentWidth * totalRound, [self = weak_from_this(), offset, currentWidth]() {
                    auto sender = self.lock();
                    if (!sender)
                    {
                        return;
                    }
                    sender->computeOprfOutputs(offset, currentWidth);
                    sender->increaseOprfRound();
                    sender->doOprf();
                });

            m_matrixProgress->reset(currentWidth * totalRound, [self = weak_from_this()]() {
                auto sender = self.lock();
                if (!sender)
                {
                    return;
                }
                sender->noticeReceiverDoNextRound();
            });

            for (uint32_t col = 0; col < currentWidth; col++)
            {
                for (uint32_t round = 0; round < totalRound; round++)
                {
                    m_channel->asyncReceiveMessage(uint8_t(CM2020PSIMessageType::MATRIX),
                        (offset + col) * totalRound + round, 30 * 60,
                        [self = weak_from_this(), offset, col, totalRound, round](
                            bcos::Error::Ptr _error, front::PPCMessageFace::Ptr _message) {
                            auto sender = self.lock();
                            if (!sender)
                            {
                                return;
                            }
                            if (_error && _error->errorCode())
                            {
                                sender->onSenderTaskDone(std::move(_error));
                            }
                            else
                            {
                                sender->matrixProgress()->mark<int64_t>(
                                    (offset + col) * totalRound + round);

                                sender->handleMatrixColumnReceived(
                                    offset, col, round, _message->data());

                                sender->progress()->mark<int64_t>(
                                    (offset + col) * totalRound + round);
                            }
                        });
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        onSenderException("doOprf", e);
    }
}

void CM2020PSISender::noticeReceiverDoNextRound()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("noticeReceiverDoNextRound") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("round", m_oprfRound);
    auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
        uint8_t(protocol::TaskAlgorithmType::CM_PSI_2PC), m_taskID,
        std::make_shared<bcos::bytes>());
    message->setMessageType(uint8_t(CM2020PSIMessageType::DO_NEXT_ROUND));
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](bcos::Error::Ptr _error) {
            auto sender = self.lock();
            if (!sender)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                sender->onSenderTaskDone(std::move(_error));
            }
        },
        nullptr);
}

void CM2020PSISender::initMatrixParams()
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
                         << LOG_KV("sInputSize", m_sInputSize)
                         << LOG_KV("bucketNumber", m_params->bucketNumber())
                         << LOG_KV("handleWidth", m_handleWidth)
                         << LOG_KV("bucketSizeInBytes", m_bucketSizeInBytes)
                         << LOG_KV("mask", m_mask);
}

void CM2020PSISender::constructMatrices(uint32_t _offset, uint32_t _width)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("constructMatrices") << LOG_KV("offset", _offset)
                         << LOG_KV("width", _width) << LOG_KV("taskID", m_taskID);

    if (_offset == 0)
    {
        m_matrixC.reserve(_width);
        m_matrixC.resize(_width);
    }

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, _width), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            // construct matrix A
            if (_offset == 0)
            {
                m_matrixC[i].reserve(m_bucketSizeInBytes);
                m_matrixC[i].resize(m_bucketSizeInBytes);
            }

            AESPRNG prng(m_receiverKeys[_offset + i]);
            prng.generate(m_matrixC[i].data(), m_bucketSizeInBytes);
        }
    });
}

void CM2020PSISender::onMatrixColumnReceived(PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("onMatrixColumnReceived") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("seq", _message->seq());
    try
    {
        m_channel->onMessageArrived(uint8_t(CM2020PSIMessageType::MATRIX), _message);
    }
    catch (const std::exception& e)
    {
        onSenderException("onMatrixColumnReceived", e);
    }
}

void CM2020PSISender::handleMatrixColumnReceived(
    uint32_t _offset, uint32_t _col, uint32_t _round, bcos::bytesPointer _buffer)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(DEBUG) << LOG_BADGE("handleMatrixColumnReceived") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("offset", _offset) << LOG_KV("col", _col)
                          << LOG_KV("round", _round);
    try
    {
        if (m_otChoices->get(_offset + _col))
        {
            uint32_t length = _buffer->size();
            uint32_t start = _round * MAX_SEND_BUFFER_LENGTH;
            for (uint32_t i = 0; i < length; i++)
            {
                m_matrixC[_col][start + i] ^= _buffer->at(i);
            }
        }
    }
    catch (const std::exception& e)
    {
        onSenderException("handleMatrixColumnReceived", e);
    }
}

void CM2020PSISender::computeOprfOutputs(uint32_t _offset, uint32_t _width)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("computeOprfOutputs") << LOG_KV("offset", _offset)
                         << LOG_KV("width", _width) << LOG_KV("taskID", m_taskID);
    try
    {
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, m_sInputSize), [&](auto const& range) {
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
                        (uint8_t)((bool)(m_matrixC[j][location >> 3] & (1 << (location & 7))))
                        << ((_offset + j) & 7);
                }
            }
        });
    }
    catch (const std::exception& e)
    {
        onSenderException("computeOprfOutputs", e);
    }
}

void CM2020PSISender::clearOprfMemory()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("clearOprfMemory") << LOG_KV("taskID", m_taskID);
    std::vector<std::array<uint32_t, 8>>().swap(m_originLocations);
    std::vector<bcos::bytes>().swap(m_matrixC);
    MallocExtension::instance()->ReleaseFreeMemory();
}

void CM2020PSISender::doPsi()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("doPsi") << LOG_KV("taskID", m_taskID);

    try
    {
        m_progress->reset(-1, nullptr);
        clearOprfMemory();
        if (m_params->enableSyncResults())
        {
            m_sResults = std::make_shared<std::vector<uint8_t>>(m_sInputSize, 0);
        }
        computeAndSendHash();
        finishPsi();
    }
    catch (const std::exception& e)
    {
        onSenderException("doPsi", e);
    }
}

void CM2020PSISender::computeAndSendHash()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(INFO) << LOG_BADGE("computeAndSendHash") << LOG_KV("taskID", m_taskID);

    // send 2^18 hashes per round
    uint32_t sendNum = MAX_SEND_BUFFER_LENGTH / RESULT_LEN_BYTE;
    uint32_t totalRound = (m_sInputSize + sendNum - 1) / sendNum;

    auto hash = m_config->hash();

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, totalRound), [&](auto const& range) {
        for (auto round = range.begin(); round < range.end(); round++)
        {
            uint32_t currentLen =
                sendNum * (round + 1) > m_sInputSize ? m_sInputSize - sendNum * round : sendNum;
            auto buffer = std::make_shared<bcos::bytes>(currentLen * RESULT_LEN_BYTE);
            uint32_t offset = round * sendNum;
            for (uint32_t i = 0; i < currentLen; i++)
            {
                auto res = hash->hash(bcos::ref(m_oprfOutputs[offset + i]));
                memcpy(buffer->data() + i * RESULT_LEN_BYTE, res.data(), RESULT_LEN_BYTE);
            }

            auto message = m_config->ppcMsgFactory()->buildPPCMessage(
                uint8_t(TaskType::PSI), uint8_t(TaskAlgorithmType::CM_PSI_2PC), m_taskID, buffer);
            message->setMessageType(uint8_t(CM2020PSIMessageType::HASHES));
            message->setSeq(round);

            if (m_params->lowBandwidth())
            {
                auto error = m_config->sendMessage(m_taskState->peerID(), message);
                if (error && error->errorCode())
                {
                    onSenderTaskDone(std::move(error));
                }
            }
            else
            {
                m_config->front()->asyncSendMessage(
                    m_taskState->peerID(), message, m_config->networkTimeout(),
                    [self = weak_from_this()](bcos::Error::Ptr _error) {
                        auto sender = self.lock();
                        if (!sender)
                        {
                            return;
                        }
                        if (_error && _error->errorCode())
                        {
                            sender->onSenderTaskDone(std::move(_error));
                        }
                    },
                    nullptr);
            }
        }
    });

    // clear oprf outputs
    std::vector<std::vector<bcos::byte>>().swap(m_oprfOutputs);
    MallocExtension::instance()->ReleaseFreeMemory();
}

void CM2020PSISender::finishPsi()
{
    if (m_taskState->taskDone())
    {
        return;
    }
    try
    {
        if (m_params->enableSyncResults())
        {
            auto end = std::chrono::high_resolution_clock::now();
            // costs of psi
            m_costs[2] =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - m_startTimePoint)
                    .count();

            CM2020_PSI_LOG(INFO) << LOG_BADGE("waiting results") << LOG_KV("taskID", m_taskID);
        }
        else
        {
            onSenderTaskDone(nullptr);
        }
    }
    catch (const std::exception& e)
    {
        onSenderException("finishPsi", e);
    }
}

void CM2020PSISender::onResultCountReceived(PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    try
    {
        decodeUnsignedNum(m_resultCount, _message->data());
        m_resultCountReceived.exchange(true);

        CM2020_PSI_LOG(INFO) << LOG_BADGE("onResultCountReceived") << LOG_KV("taskID", m_taskID)
                             << LOG_KV("count", m_resultCount);

        uint32_t number = MAX_SEND_BUFFER_LENGTH / sizeof(uint32_t);
        uint32_t totalRound = (m_resultCount + number - 1) / number + 1;

        if (m_progress->mark<int64_t>(-1) == totalRound)
        {
            if (m_resultCount > 0)
            {
                saveResults();
            }
            onSenderTaskDone(nullptr);
        }
    }
    catch (const std::exception& e)
    {
        onSenderException("onResultCountReceived", e);
    }
}

void CM2020PSISender::onResultReceived(PPCMessageFace::Ptr _message)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    CM2020_PSI_LOG(DEBUG) << LOG_BADGE("onResultReceived") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("seq", _message->seq());
    try
    {
        auto buffer = _message->data();
        uint32_t resultNum = buffer->size() / sizeof(uint32_t);
        for (uint32_t i = 0; i < resultNum; i++)
        {
            uint32_t index;
            decodeUnsignedNum(index, buffer->data() + i * sizeof(uint32_t));
            m_sResults->at(index) = 1;
        }

        if (m_resultCountReceived)
        {
            // receive 2^20 index per round
            uint32_t number = MAX_SEND_BUFFER_LENGTH / sizeof(uint32_t);
            uint32_t totalRound = (m_resultCount + number - 1) / number + 1;

            if (m_progress->mark<int64_t>(_message->seq()) == totalRound)
            {
                if (m_resultCount > 0)
                {
                    saveResults();
                }

                onSenderTaskDone(nullptr);
            }
        }
        else
        {
            m_progress->mark<int64_t>(_message->seq());
        }
    }
    catch (const std::exception& e)
    {
        onSenderException("onResultReceived", e);
    }
}

void CM2020PSISender::saveResults()
{
    CM2020_PSI_LOG(INFO) << LOG_BADGE("saveResults") LOG_KV("taskID", m_taskID);
    try
    {
        if (m_ioByInterface)
        {
            std::vector<std::string> results;
            for (uint32_t index = 0; index < m_sInputSize; index++)
            {
                if (m_sResults->at(index))
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
            for (uint32_t index = 0; index < m_sInputSize; index++)
            {
                if (m_sResults->at(index))
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
        onSenderException("saveResults", e);
    }
}

void CM2020PSISender::onSenderException(const std::string& _message, const std::exception& e)
{
    CM2020_PSI_LOG(ERROR) << LOG_BADGE(_message) << LOG_KV("taskID", m_taskID)
                          << LOG_KV("exception", boost::diagnostic_information(e));
    auto error = std::make_shared<bcos::Error>(
        (int)CM2020PSIRetCode::ON_EXCEPTION, boost::diagnostic_information(e));
    onSenderTaskDone(error);
}

void CM2020PSISender::onSenderTaskDone(bcos::Error::Ptr _error)
{
    if (m_taskState->taskDone())
    {
        return;
    }

    CM2020_PSI_LOG(INFO) << LOG_BADGE("onSenderTaskDone") << LOG_KV("taskID", m_taskID);
    m_channelManager->removeChannelByTask(m_taskID);

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

        if (m_resultCount)
        {
            communication += (uint64_t)m_resultCount * (uint64_t)sizeof(uint32_t);
        }

        communication /= (uint64_t)(1024 * 1024);

        //        message = "\nStatus: SUCCESS\nOrigin Inputs Number: " +
        //        std::to_string(m_sInputSize) +
        //                  "\nCounterparty Inputs Number: " + std::to_string(m_rInputSize) +
        //                  "\nIntersections Number: " + std::to_string(m_resultCount) +
        //                  "\nCommunication: " + std::to_string(communication) +
        //                  "MB\nTotal Costs: " + std::to_string(m_costs[3]) + "ms\n";
        message = "\nStatus: SUCCESS\nOrigin Inputs Number: " + std::to_string(m_sInputSize) +
                  "\nIntersections Number: " + std::to_string(m_resultCount) +
                  "\nCommunication: " + std::to_string(communication) +
                  "MB\nTotal Costs: " + std::to_string(m_costs[3]) + "ms\n";

        m_cm2020Result->m_enableSyncResults = m_params->enableSyncResults();
        m_cm2020Result->m_bucketNumber = m_params->bucketNumber();
        m_cm2020Result->m_communication = std::to_string(communication) + "MB";
        m_cm2020Result->m_intersections = m_resultCount;
        m_cm2020Result->m_party0Size = m_rInputSize;
        m_cm2020Result->m_party1Size = m_sInputSize;
    }

    m_taskState->onTaskFinished(m_cm2020Result, true);

    CM2020_PSI_LOG(INFO) << LOG_BADGE("onSenderTaskDone") << LOG_KV("taskID", m_taskID)
                         << LOG_KV("result", message);
}
