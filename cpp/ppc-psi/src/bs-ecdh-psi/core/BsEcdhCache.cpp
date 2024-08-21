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
 * @file BsEcdhCache.cpp
 * @author: shawnhe
 * @date 2023-09-20
 */

#include "BsEcdhCache.h"

using namespace bcos;
using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;

void BsEcdhCache::start()
{
    m_threadPool->enqueue([self = weak_from_this()]() {
        auto cache = self.lock();
        if (!cache)
        {
            return;
        }
        cache->prepareCipher();
    });
}

void BsEcdhCache::generateKey()
{
    m_key.resize(SCALAR_SIZE + 1);
    COutputBuffer outputBuffer{(char*)m_key.data(), SCALAR_SIZE};
    auto ret = wedpr_random_scalar(&outputBuffer);
    if (ret != WEDPR_SUCCESS)
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR(
            (int64_t)PSIRetCode::OnException, "Generate key error, code: " + std::to_string(ret)));
    }
    m_key.resize(outputBuffer.len);
}

std::string BsEcdhCache::genCipherWithBase64(const std::string& _input)
{
    std::string hash(POINT_SIZE + 1, 0);
    CInputBuffer message{(char*)_input.data(), _input.size()};
    COutputBuffer point{(char*)hash.data(), POINT_SIZE};
    auto ret = wedpr_hash_to_curve(&message, &point);
    if (ret != WEDPR_SUCCESS)
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR(
            (int64_t)PSIRetCode::OnException, "Hash to curve error, code: " + std::to_string(ret)));
    }

    CInputBuffer key{(char*)m_key.data(), m_key.size()};
    std::string cipher(POINT_SIZE + 1, 0);
    CInputBuffer hashPoint{(char*)point.data, point.len};
    COutputBuffer cipherPoint{(char*)cipher.data(), cipher.size()};
    ret = wedpr_point_scalar_multi(&hashPoint, &key, &cipherPoint);
    if (ret != WEDPR_SUCCESS)
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PSIRetCode::OnException,
            "Point multiply by scalar, code: " + std::to_string(ret)));
    }
    return base64Encode(bytesConstRef((byte*)cipherPoint.data, cipherPoint.len));
}

std::string BsEcdhCache::genEcdhCipherWithBase64(const std::string& _point)
{
    auto point = base64Decode(_point);
    CInputBuffer rawPoint{(char*)point.data(), point.size()};
    CInputBuffer key{(char*)m_key.data(), m_key.size()};
    std::string ecdhCipher(POINT_SIZE + 1, 0);
    COutputBuffer ecdhCipherPoint{(char*)ecdhCipher.data(), ecdhCipher.size()};
    auto ret = wedpr_point_scalar_multi(&rawPoint, &key, &ecdhCipherPoint);
    if (ret != WEDPR_SUCCESS)
    {
        BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PSIRetCode::OnException,
            "Point multiply by scalar, code: " + std::to_string(ret)));
    }
    return base64Encode(bytesConstRef((byte*)ecdhCipherPoint.data, ecdhCipherPoint.len));
}

void BsEcdhCache::prepareIoHandler()
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start preparing io handler") << LOG_KV("taskID", m_taskID);

    auto reader = m_taskGuarder->loadReader(m_taskID, m_dataResource, DataSchema::String);

    auto resultWriter = m_taskGuarder->loadWriter(m_taskID, m_dataResource, m_enableOutputExists);

    auto outputPath = m_dataResource->outputDesc()->path();
    auto indexPath = outputPath + INDEX_FILE_SUFFIX;
    m_dataResource->mutableOutputDesc()->setPath(indexPath);
    auto indexWriter = m_taskGuarder->loadWriter(m_taskID, m_dataResource, m_enableOutputExists);

    LineWriter::Ptr evidenceWriter = nullptr;
    if (m_enableAudit)
    {
        auto evidencePath = outputPath + AUDIT_FILE_SUFFIX;
        m_dataResource->mutableOutputDesc()->setPath(evidencePath);
        evidenceWriter = m_taskGuarder->loadWriter(m_taskID, m_dataResource, m_enableOutputExists);
    }

    m_ioHandler = std::make_shared<BsEcdhIoHandler>(
        m_taskID, reader, resultWriter, indexWriter, evidenceWriter);

    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish preparing io handler") << LOG_KV("taskID", m_taskID);
}

void BsEcdhCache::prepareCipher()
{
    try
    {
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("start preparing cipher") << LOG_KV("taskID", m_taskID);

        prepareIoHandler();

        // generate ecc sk
        generateKey();
        // write key into evidence file
        m_ioHandler->appendEvidence(
            "WB KEY", base64Encode(bytesConstRef(m_key.data(), m_key.size())));

        m_originInputs = m_ioHandler->loadInputs();
        if (!m_originInputs || m_originInputs->size() == 0)
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int64_t)PSIRetCode::OnException, "Data is empty."));
        }
        m_inputsSize = m_originInputs->size();
        m_ciphers.reserve(m_inputsSize);
        m_ciphers.resize(m_inputsSize);
        m_ecdhCiphers.reserve(m_inputsSize);
        m_ecdhCiphers.resize(m_inputsSize);
        m_ecdhCiphersMap.reserve(m_inputsSize);
        m_ecdhCipherFlags.assign(m_inputsSize, false);

        tbb::parallel_for(tbb::blocked_range<size_t>(0U, m_inputsSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                m_ciphers[i] = std::move(genCipherWithBase64(m_originInputs->get<std::string>(i)));
            }
        });

        // write ciphers into evidence file
        m_ioHandler->appendEvidences("WB CIPHERS", m_ciphers);

        if (m_onSelfCiphersReady)
        {
            m_step = ProcessingSelfCiphers;
            m_onSelfCiphersReady();
        }

        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("finish preparing cipher") << LOG_KV("taskID", m_taskID);
    }
    catch (const std::exception& e)
    {
        onSelfException("prepareCipher", e);
    }
}

uint32_t BsEcdhCache::findCurrentIndex(
    const std::vector<bool>& _flags, uint32_t _offset, uint32_t _total)
{
    uint32_t final = _offset;
    do
    {
        final++;
    } while (_flags[final] && (final < _total));

    return final;
}

BsEcdhResult::Ptr BsEcdhCache::fetchCipher(FetchCipherRequest::Ptr _request)
{
    try
    {
        ReadGuard l(x_ecdhCiphers);
        if (m_selfEcdhCiphersReady)
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)PSIRetCode::OnException, "WB ciphers hava been processed."));
        }

        auto response = std::make_shared<FetchCipherResponse>();
        response->taskID = m_taskID;
        response->offset = _request->offset;
        response->total = m_inputsSize;
        response->ciphers.clear();

        uint32_t end = _request->offset + _request->size >= m_inputsSize ?
                           m_inputsSize :
                           _request->offset + _request->size;
        for (uint32_t i = _request->offset; i < end; i++)
        {
            response->ciphers.push_back(m_ciphers[i]);
        }
        response->size = response->ciphers.size();

        return std::make_shared<BsEcdhResult>(m_taskID, response->serialize());
    }
    catch (const std::exception& e)
    {
        return onRequestException("fetchCipher", e);
    }
}

BsEcdhResult::Ptr BsEcdhCache::onEcdhCipherReceived(SendEcdhCipherRequest::Ptr _request)
{
    try
    {
        WriteGuard l(x_ecdhCiphers);
        if (m_selfEcdhCiphersReady)
        {
            return std::make_shared<BsEcdhResult>(m_taskID);
        }

        // save ecdh ciphers
        uint32_t end = _request->offset + _request->ecdhCiphers.size() >= m_inputsSize ?
                           m_inputsSize :
                           _request->offset + _request->ecdhCiphers.size();
        for (uint32_t i = _request->offset; i < end; i++)
        {
            if (!m_ecdhCipherFlags[i])
            {
                m_ecdhCipherFlags[i] = true;
                m_receivedEcdhCipherCount++;
            }
            m_ecdhCiphers[i] = _request->ecdhCiphers[i - _request->offset];
        }
        auto total =
            m_inputsSize + m_partnerInputsSize == 0 ? 1 : m_inputsSize + m_partnerInputsSize;
        uint64_t currentCount = m_receivedPartnerCipherCount + m_receivedEcdhCipherCount;
        m_progress = currentCount * 100 / total;
        if (m_progress % 10 == 0)
        {
            BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onEcdhCipherReceived")
                                  << LOG_KV("totalReceived", currentCount)
                                  << LOG_KV("progress", m_progress) << LOG_KV("taskID", m_taskID);
        }

        if (_request->offset == m_selfIndex)
        {
            m_selfIndex = findCurrentIndex(m_ecdhCipherFlags, _request->offset, m_inputsSize);
        }

        if (m_receivedEcdhCipherCount == m_inputsSize)
        {
            m_step = ProcessingPartnerCiphers;
            m_selfEcdhCiphersReady.exchange(true);
            m_threadPool->enqueue([self = weak_from_this()]() {
                auto cache = self.lock();
                if (!cache)
                {
                    return;
                }
                cache->onAllSelfEcdhCiphersReady();
            });
        }

        return std::make_shared<BsEcdhResult>(m_taskID);
    }
    catch (const std::exception& e)
    {
        return onRequestException("onEcdhCipherReceived", e);
    }
}

BsEcdhResult::Ptr BsEcdhCache::onPartnerCipherReceived(SendPartnerCipherRequest::Ptr _request)
{
    try
    {
        WriteGuard l(x_partnerCiphers);
        if (m_partnerEcdhCiphersReady)
        {
            return std::make_shared<BsEcdhResult>(m_taskID);
        }

        if (m_partnerCiphers.empty())
        {
            if (m_partnerInputsSize == 0)
            {
                // compatible with older versions
                m_partnerInputsSize = _request->total;
            }
            m_partnerCiphers.reserve(m_partnerInputsSize);
            m_partnerCiphers.resize(m_partnerInputsSize);
            m_partnerEcdhCiphers.reserve(m_partnerInputsSize);
            m_partnerEcdhCiphers.resize(m_partnerInputsSize);
            m_partnerCipherFlags.assign(m_partnerInputsSize, false);
        }

        // save partner ciphers
        uint32_t end = _request->offset + _request->partnerCiphers.size() >= m_partnerInputsSize ?
                           m_partnerInputsSize :
                           _request->offset + _request->partnerCiphers.size();
        for (uint32_t i = _request->offset; i < end; i++)
        {
            if (!m_partnerCipherFlags[i])
            {
                m_partnerCipherFlags[i] = true;
                m_receivedPartnerCipherCount++;
            }

            m_partnerCiphers[i] = _request->partnerCiphers[i - _request->offset];
        }

        auto total =
            m_inputsSize + m_partnerInputsSize == 0 ? 1 : m_inputsSize + m_partnerInputsSize;
        uint64_t currentCount = m_receivedPartnerCipherCount + m_receivedEcdhCipherCount;
        m_progress = currentCount * 100 / total;
        if (m_progress % 10 == 0)
        {
            BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onPartnerCipherReceived")
                                  << LOG_KV("totalReceived", currentCount)
                                  << LOG_KV("progress", m_progress) << LOG_KV("taskID", m_taskID);
        }

        bool error = false;
        // compute partner ecdh ciphers
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0U, end - _request->offset), [&](auto const& range) {
                for (auto i = range.begin(); i < range.end(); i++)
                {
                    auto index = _request->offset + i;
                    try
                    {
                        m_partnerEcdhCiphers[index] =
                            std::move(genEcdhCipherWithBase64(_request->partnerCiphers[i]));
                    }
                    catch (const std::exception& e)
                    {
                        error = true;
                        BS_ECDH_PSI_LOG(WARNING)
                            << LOG_DESC("genEcdhCipherWithBase64")
                            << LOG_KV("cipher", _request->partnerCiphers[i])
                            << LOG_KV("exception", boost::diagnostic_information(e));
                    }
                }
            });

        if (error)
        {
            BOOST_THROW_EXCEPTION(
                BCOS_ERROR((int64_t)PSIRetCode::OnException, "Compute partner ecdh ciphers error"));
        }

        if (_request->offset == m_partnerIndex)
        {
            m_partnerIndex =
                findCurrentIndex(m_partnerCipherFlags, _request->offset, m_partnerInputsSize);
        }

        if (m_receivedPartnerCipherCount == m_partnerInputsSize)
        {
            m_partnerEcdhCiphersReady.exchange(true);
            m_threadPool->enqueue([self = weak_from_this()]() {
                auto cache = self.lock();
                if (!cache)
                {
                    return;
                }
                cache->onAllPartnerEcdhCiphersReady();
            });
        }

        return std::make_shared<BsEcdhResult>(m_taskID);
    }
    catch (const std::exception& e)
    {
        return onRequestException("onPartnerCipherReceived", e);
    }
}

void BsEcdhCache::onAllSelfEcdhCiphersReady()
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onAllSelfEcdhCiphersReady") << LOG_KV("taskID", m_taskID);

    for (uint32_t i = 0; i < m_inputsSize; i++)
    {
        m_ecdhCiphersMap[m_ecdhCiphers[i]] = i;
    }

    // write ecdh ciphers into evidence file
    m_ioHandler->appendEvidences("WB ECDH CIPHERS", m_ecdhCiphers);

    // release memory
    std::vector<std::string>().swap(m_ciphers);
    std::vector<std::string>().swap(m_ecdhCiphers);
    std::vector<bool>().swap(m_ecdhCipherFlags);
    MallocExtension::instance()->ReleaseFreeMemory();

    if (m_partnerEcdhCiphersReady)
    {
        onAllEcdhCiphersReady();
    }
}

void BsEcdhCache::onAllPartnerEcdhCiphersReady()
{
    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onAllPartnerEcdhCiphersReady") << LOG_KV("taskID", m_taskID);

    if (m_selfEcdhCiphersReady)
    {
        onAllEcdhCiphersReady();
    }
}

void BsEcdhCache::onAllEcdhCiphersReady()
{
    if (m_allCiphersReady.exchange(true))
    {
        return;
    }

    BS_ECDH_PSI_LOG(INFO) << LOG_DESC("onAllCiphersReady") << LOG_KV("taskID", m_taskID);
    if (m_onAllCiphersReady)
    {
        m_onAllCiphersReady();
    }

    m_step = ComputingResults;

    try
    {
        // write partner ciphers into evidence file at the end
        m_ioHandler->appendEvidences("PARTNER CIPHERS", m_partnerCiphers);

        // release memory
        std::vector<std::string>().swap(m_partnerCiphers);
        std::vector<bool>().swap(m_partnerCipherFlags);
        MallocExtension::instance()->ReleaseFreeMemory();

        m_ioHandler->uploadEvidences();

        // find intersections
        DataBatch::Ptr partnerIndexes = std::make_shared<DataBatch>();
        std::unordered_set<std::string> intersections;
        uint32_t dumpSize = 0;
        for (uint32_t i = 0; i < m_partnerInputsSize; i++)
        {
            if (m_ecdhCiphersMap.contains(m_partnerEcdhCiphers[i]))
            {
                auto index = m_ecdhCiphersMap[m_partnerEcdhCiphers[i]];
                auto data = m_originInputs->get<std::string>(index);
                if (!intersections.contains(data))
                {
                    intersections.insert(data);
                    partnerIndexes->append<std::string>(std::to_string(i));
                }
                else
                {
                    dumpSize++;
                }
            }
        }

        // release memory
        if (m_originInputs)
        {
            m_originInputs->setData(std::vector<bcos::bytes>());
        }
        std::unordered_map<std::string, uint32_t>().swap(m_ecdhCiphersMap);
        std::vector<std::string>().swap(m_partnerEcdhCiphers);
        MallocExtension::instance()->ReleaseFreeMemory();

        auto resultCount = intersections.size();
        m_ioHandler->saveResults(intersections);
        m_ioHandler->saveIndexes(partnerIndexes);

        std::unordered_set<std::string>().swap(intersections);
        partnerIndexes->setData(std::vector<bcos::bytes>());
        MallocExtension::instance()->ReleaseFreeMemory();

        BS_ECDH_PSI_LOG(INFO) << LOG_DESC(
                                     "finish computing results and start constructing response")
                              << LOG_KV("taskID", m_taskID);

        // construct response
        auto response = std::make_shared<GetTaskStatusResponse>();
        response->taskID = m_taskID;
        response->status = toString(TaskStatus::COMPLETED);
        response->intersections = resultCount;
        response->party0Size = m_inputsSize;
        response->party1Size = m_partnerInputsSize;
        auto timeCosts = std::to_string(utcSteadyTime() - m_startTime) + "ms";
        response->timeCost = timeCosts;
        response->step = DownloadIndex;
        response->progress = 100;

        auto result = std::make_shared<BsEcdhResult>(m_taskID, response->serialize());
        if (m_onTaskFinished)
        {
            m_onTaskFinished(TaskStatus::COMPLETED, result);
        }

        BS_ECDH_PSI_LOG(INFO) << LOG_BADGE("BsEcdhPsiTaskDone") << LOG_KV("taskID", m_taskID)
                              << LOG_KV("inputsSize", m_inputsSize)
                              << LOG_KV("partnerInputsSize", m_partnerInputsSize)
                              << LOG_KV("resultCount", resultCount) << LOG_KV("dumpSize", dumpSize)
                              << LOG_KV("timeCost", timeCosts)
                              << LOG_KV("resultFileID", response->resultFileID)
                              << LOG_KV("resultFileMd5", response->resultFileMd5)
                              << LOG_KV("partnerIndexFileID", response->partnerIndexFileID)
                              << LOG_KV("partnerIndexFileMd5", response->partnerIndexFileMd5)
                              << LOG_KV("evidenceFileID", response->evidenceFileID)
                              << LOG_KV("evidenceFileMd5", response->evidenceFileMd5);
    }
    catch (const std::exception& e)
    {
        return onSelfException("onAllCiphersReady", e);
    }
}

void BsEcdhCache::onSelfException(const std::string& _module, const std::exception& e)
{
    BS_ECDH_PSI_LOG(ERROR) << LOG_DESC("onSelfException") << LOG_KV("taskID", m_taskID)
                           << LOG_KV("module", _module)
                           << LOG_KV("exception", boost::diagnostic_information(e));

    GetTaskStatusResponse response;
    response.taskID = m_taskID;
    response.status = toString(TaskStatus::FAILED);
    auto result = std::make_shared<BsEcdhResult>(m_taskID, response.serialize());
    result->setError(std::make_shared<bcos::Error>(
        (int)PSIRetCode::OnException, "Task failed: " + boost::diagnostic_information(e)));

    if (m_onTaskFinished)
    {
        m_onTaskFinished(TaskStatus::FAILED, result);
    }
}

BsEcdhResult::Ptr BsEcdhCache::onRequestException(
    const std::string& _module, const std::exception& e)
{
    BS_ECDH_PSI_LOG(WARNING) << LOG_DESC("onRequestException") << LOG_KV("taskID", m_taskID)
                             << LOG_KV("module", _module)
                             << LOG_KV("exception", boost::diagnostic_information(e));

    auto result = std::make_shared<BsEcdhResult>(m_taskID);
    result->setError(std::make_shared<bcos::Error>(
        (int)PSIRetCode::OnException, "Task failed: " + boost::diagnostic_information(e)));
    return result;
}
