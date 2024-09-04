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
 * @file CM2020PSIReceiver.h
 * @author: shawnhe
 * @date 2022-12-7
 */

#pragma once
#include "../../psi-framework/TaskState.h"
#include "../protocol/CM2020PSIResult.h"
#include "TaskParams.h"
#include "ppc-crypto/src/randomot/SimplestOT.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-psi/src/cm2020-psi/CM2020PSIConfig.h"
#include "ppc-tools/src/common/Progress.h"
#include <gperftools/malloc_extension.h>
#include <memory>

namespace ppc::psi
{
class CM2020PSIReceiver : public std::enable_shared_from_this<CM2020PSIReceiver>
{
public:
    using Ptr = std::shared_ptr<CM2020PSIReceiver>;

    CM2020PSIReceiver(
        CM2020PSIConfig::Ptr m_config, TaskState::Ptr _taskState, crypto::SimplestOT::Ptr _ot);

    ~CM2020PSIReceiver()
    {
        if (m_originInputs)
        {
            m_originInputs->setData(std::vector<bcos::bytes>());
        }
        std::vector<std::array<uint32_t, 8>>().swap(m_originLocations);
        std::vector<std::vector<bcos::byte>>().swap(m_oprfOutputs);
        std::vector<bcos::bytes>().swap(m_matrixA);
        std::vector<bcos::bytes>().swap(m_matrixDelta);
        std::unordered_map<u128, uint32_t>().swap(m_hashes);
        std::unordered_map<uint32_t, bcos::bytesPointer>().swap(m_receivedHash);
        std::shared_ptr<std::vector<uint8_t>>().swap(m_rResults);
        std::shared_ptr<std::vector<uint8_t>>().swap(m_sResults);
        // release the memory to os
        MallocExtension::instance()->ReleaseFreeMemory();
        CM2020_PSI_LOG(INFO) << LOG_DESC("the receiver destroyed") << LOG_KV("taskID", m_taskID);
    }

    void asyncRunTask();

    void onHandshakeDone();

    void onSenderSizeReceived(front::PPCMessageFace::Ptr _message);

    void onBatchPointBReceived(front::PPCMessageFace::Ptr _message);

    void onDoNextRoundReceived();

    void onHashesReceived(front::PPCMessageFace::Ptr _message);

    const std::string& taskID() const { return m_taskID; }
    TaskParams::Ptr params() { return m_params; }

protected:
    void runReceiver();
    void prepareInputs();
    void syncInputsSize();
    void runRandomOT();

    void preprocessInputs();
    void doOprf();
    void initMatrixParams();
    void constructMatrices(uint32_t _offset, uint32_t _width);
    void computeOprfOutputs(uint32_t _offset, uint32_t _width);
    void negotiateMatrix(uint32_t _bucketIndex, uint32_t _matrixIndex);
    void increaseOprfRound() { m_oprfRound++; }
    void clearOprfMemory();

    void doPsi();
    void computeHash();
    void tryComputeIntersection();
    void computeIntersection(uint32_t _round, bcos::bytesPointer _buffer);

    void finishPsi();
    void syncResults(uint32_t _count);
    void saveResults();

    void onReceiverException(const std::string& _module, const std::exception& e);
    void onReceiverTaskDone(bcos::Error::Ptr _error);

    tools::Progress::Ptr progress() { return m_progress; }

private:
    CM2020PSIConfig::Ptr m_config;
    TaskState::Ptr m_taskState;
    crypto::SimplestOT::Ptr m_ot;

    std::string m_taskID;
    TaskParams::Ptr m_params;
    CM2020PSIResult::Ptr m_cm2020Result;

    ppc::io::DataBatch::Ptr m_originInputs;
    uint32_t m_rInputSize;
    uint32_t m_sInputSize;
    bool m_ioByInterface{false};

    std::pair<bcos::bytes, bcos::bytesPointer> m_otState;
    std::vector<std::array<bcos::bytes, 2>> m_senderKeys;

    std::vector<std::array<uint32_t, 8>> m_originLocations;
    std::vector<std::vector<bcos::byte>> m_oprfOutputs;

    // width of per round processing
    uint32_t m_handleWidth;

    // size of each bucket (bytes)
    // the bucket size is also the height of matrix
    uint32_t m_bucketSizeInBytes;
    uint32_t m_mask;

    std::atomic<uint32_t> m_oprfRound{0};
    uint32_t m_currentWidth;
    std::vector<bcos::bytes> m_matrixA;
    std::vector<bcos::bytes> m_matrixDelta;

    // key: the hash of OPRF output, value: the index of input
    std::unordered_map<u128, uint32_t> m_hashes;

    std::atomic<bool> m_hashReady{false};
    uint32_t m_maxHashSeq;
    mutable bcos::SharedMutex x_receivedHash;
    std::unordered_map<uint32_t, bcos::bytesPointer> m_receivedHash;

    std::atomic<uint32_t> m_resultCount;
    std::shared_ptr<std::vector<uint8_t>> m_rResults;
    std::shared_ptr<std::vector<uint8_t>> m_sResults;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimePoint;
    std::vector<long long> m_costs;

    tools::Progress::Ptr m_progress;
};
}  // namespace ppc::psi
