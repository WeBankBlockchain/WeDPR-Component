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
 * @file BsEcdhCache.h
 * @author: shawnhe
 * @date 2023-09-20
 */

#pragma once

#include "BsEcdhIoHandler.h"
#include "ppc-psi/src/Common.h"
#include "ppc-psi/src/bs-ecdh-psi/Common.h"
#include "ppc-psi/src/bs-ecdh-psi/ffi/wedpr_ffi_edwards25519.h"
#include "ppc-psi/src/bs-ecdh-psi/protocol/BsEcdhResult.h"
#include "ppc-psi/src/bs-ecdh-psi/protocol/Message.h"
#include "ppc-psi/src/psi-framework/TaskGuarder.h"
#include <bcos-utilities/Base64.h>
#include <bcos-utilities/ThreadPool.h>
#include <gperftools/malloc_extension.h>
#include <tbb/parallel_for.h>

namespace ppc::psi
{
enum TaskStep : int
{
    Initializing = 1,
    ProcessingSelfCiphers,
    ProcessingPartnerCiphers,
    ComputingResults,
    // execution in ppcs-adm
    DownloadIndex,
};

class BsEcdhCache : public std::enable_shared_from_this<BsEcdhCache>
{
public:
    using Ptr = std::shared_ptr<BsEcdhCache>;
    BsEcdhCache(std::string _taskID, bcos::ThreadPool::Ptr _threadPool,
        TaskGuarder::Ptr _taskGuarder, protocol::DataResource::Ptr _dataResource, bool _enableAudit,
        bool _enableOutputExists, std::function<void()>&& _onSelfCiphersReady,
        std::function<void()>&& _onAllCiphersReady,
        std::function<void(protocol::TaskStatus, BsEcdhResult::Ptr)>&& _onTaskFinished,
        uint32_t _partnerInputsSize)
      : m_taskID(std::move(_taskID)),
        m_threadPool(std::move(_threadPool)),
        m_taskGuarder(std::move(_taskGuarder)),
        m_dataResource(std::move(_dataResource)),
        m_enableAudit(_enableAudit),
        m_enableOutputExists(_enableOutputExists),
        m_onSelfCiphersReady(std::move(_onSelfCiphersReady)),
        m_onAllCiphersReady(std::move(_onAllCiphersReady)),
        m_onTaskFinished(std::move(_onTaskFinished)),
        m_partnerInputsSize(_partnerInputsSize)
    {
        m_startTime = bcos::utcSteadyTime();
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("create BsEcdhCache")
                              << LOG_KV("partnerInputsSize", m_partnerInputsSize)
                              << LOG_KV("taskID", m_taskID);
    }
    ~BsEcdhCache()
    {
        if (m_ioHandler)
        {
            m_ioHandler->clean();
        }
        // release the memory to os
        std::vector<std::string>().swap(m_ciphers);
        std::vector<std::string>().swap(m_ecdhCiphers);
        std::vector<bool>().swap(m_ecdhCipherFlags);
        std::vector<std::string>().swap(m_partnerCiphers);
        std::vector<bool>().swap(m_partnerCipherFlags);
        std::unordered_map<std::string, uint32_t>().swap(m_ecdhCiphersMap);
        std::vector<std::string>().swap(m_partnerEcdhCiphers);
        if (m_originInputs)
        {
            m_originInputs->setData(std::vector<bcos::bytes>());
        }
        MallocExtension::instance()->ReleaseFreeMemory();
        BS_ECDH_PSI_LOG(INFO) << LOG_DESC("the BsEcdhCache destroyed")
                              << LOG_KV("taskID", m_taskID);
    }

    void start();

    [[nodiscard]] std::string taskID() const { return m_taskID; }

    [[nodiscard]] uint32_t step() const { return m_step; }

    [[nodiscard]] uint32_t index() const
    {
        if (m_step == ProcessingSelfCiphers)
        {
            return m_selfIndex;
        }
        else if (m_step == ProcessingPartnerCiphers)
        {
            return m_partnerIndex;
        }
        else
        {
            return 0;
        }
    }

    [[nodiscard]] int progress() const { return m_progress; }

    BsEcdhResult::Ptr fetchCipher(FetchCipherRequest::Ptr _request);

    BsEcdhResult::Ptr onEcdhCipherReceived(SendEcdhCipherRequest::Ptr _request);

    BsEcdhResult::Ptr onPartnerCipherReceived(SendPartnerCipherRequest::Ptr _request);

public:
    // for unit test
    bcos::bytes key() { return m_key; }
    void setKey(const bcos::bytes& _key) { m_key.assign(_key.begin(), _key.end()); }

    void generateKey();
    std::string genCipherWithBase64(const std::string& _input);
    std::string genEcdhCipherWithBase64(const std::string& _point);

private:
    void prepareIoHandler();
    void prepareCipher();
    static uint32_t findCurrentIndex(
        const std::vector<bool>& _flags, uint32_t _offset, uint32_t _total);
    void onAllSelfEcdhCiphersReady();
    void onAllPartnerEcdhCiphersReady();
    void onAllEcdhCiphersReady();
    void onSelfException(const std::string& _module, const std::exception& e);
    BsEcdhResult::Ptr onRequestException(const std::string& _module, const std::exception& e);

private:
    std::string m_taskID;
    bcos::ThreadPool::Ptr m_threadPool;
    TaskGuarder::Ptr m_taskGuarder;
    protocol::DataResource::Ptr m_dataResource;
    bool m_enableAudit;
    bool m_enableOutputExists;
    BsEcdhIoHandler::Ptr m_ioHandler;
    std::function<void()> m_onSelfCiphersReady;
    std::function<void()> m_onAllCiphersReady;
    std::function<void(protocol::TaskStatus, BsEcdhResult::Ptr)> m_onTaskFinished;

    ppc::io::DataBatch::Ptr m_originInputs;
    uint32_t m_inputsSize{0};
    uint32_t m_partnerInputsSize;
    uint64_t m_startTime;

    bcos::bytes m_key;
    std::vector<std::string> m_ciphers;

    mutable bcos::SharedMutex x_ecdhCiphers;
    std::vector<std::string> m_ecdhCiphers;
    std::unordered_map<std::string, uint32_t> m_ecdhCiphersMap;
    std::vector<bool> m_ecdhCipherFlags;
    uint32_t m_receivedEcdhCipherCount{0};
    std::atomic<bool> m_selfEcdhCiphersReady{false};

    mutable bcos::SharedMutex x_partnerCiphers;
    std::vector<std::string> m_partnerCiphers;
    std::vector<bool> m_partnerCipherFlags;
    uint32_t m_receivedPartnerCipherCount{0};

    std::vector<std::string> m_partnerEcdhCiphers;
    std::atomic<bool> m_partnerEcdhCiphersReady{false};

    std::atomic<uint32_t> m_step{Initializing};
    std::atomic<uint32_t> m_selfIndex{0};
    std::atomic<uint32_t> m_partnerIndex{0};
    std::atomic<uint> m_progress{0};

    std::atomic<bool> m_allCiphersReady{false};
};

}  // namespace ppc::psi
