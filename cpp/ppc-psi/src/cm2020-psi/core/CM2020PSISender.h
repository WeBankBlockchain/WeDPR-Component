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
 * @file CM2020PSISender.h
 * @author: shawnhe
 * @date 2022-12-7
 */

#pragma once
#include "../../psi-framework/TaskState.h"
#include "../protocol/CM2020PSIResult.h"
#include "TaskParams.h"
#include "ppc-crypto/src/randomot/SimplestOT.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-front/ppc-front/PPCChannelManager.h"
#include "ppc-psi/src/cm2020-psi/CM2020PSIConfig.h"
#include "ppc-tools/src/common/Progress.h"
#include <gperftools/malloc_extension.h>
#include <memory>


namespace ppc::psi
{
class CM2020PSISender : public std::enable_shared_from_this<CM2020PSISender>
{
public:
    using Ptr = std::shared_ptr<CM2020PSISender>;
    CM2020PSISender(CM2020PSIConfig::Ptr _config, TaskState::Ptr _taskState,
        std::shared_ptr<boost::asio::io_service> _ioService, crypto::SimplestOT::Ptr _ot,
        std::shared_ptr<bcos::ThreadPool> _threadPool);

    ~CM2020PSISender()
    {
        if (m_originInputs)
        {
            m_originInputs->setData(std::vector<bcos::bytes>());
        }
        std::vector<std::array<uint32_t, 8>>().swap(m_originLocations);
        std::vector<std::vector<bcos::byte>>().swap(m_oprfOutputs);
        std::vector<bcos::bytes>().swap(m_matrixC);
        std::shared_ptr<std::vector<uint8_t>>().swap(m_sResults);
        // release the memory to os
        MallocExtension::instance()->ReleaseFreeMemory();
        CM2020_PSI_LOG(INFO) << LOG_DESC("the sender destroyed") << LOG_KV("taskID", m_taskID);
    }

    void asyncRunTask();

    void onHandshakeDone(front::PPCMessageFace::Ptr _message);

    void onReceiverSizeReceived(front::PPCMessageFace::Ptr _message);

    void onPointAReceived(front::PPCMessageFace::Ptr _message);

    void onMatrixColumnReceived(front::PPCMessageFace::Ptr _message);

    void onResultCountReceived(front::PPCMessageFace::Ptr _message);

    void onResultReceived(front::PPCMessageFace::Ptr _message);

    const std::string& taskID() const { return m_taskID; }
    TaskParams::Ptr params() { return m_params; }

protected:
    void runSender();
    void prepareInputs();
    void syncInputsSize();
    void preprocessInputs();

    void doOprf();
    void initMatrixParams();
    void constructMatrices(uint32_t _offset, uint32_t _width);

    void handleMatrixColumnReceived(
        uint32_t _offset, uint32_t _col, uint32_t _round, bcos::bytesPointer _buffer);
    void noticeReceiverDoNextRound();
    void computeOprfOutputs(uint32_t _offset, uint32_t _width);
    void clearOprfMemory();

    void doPsi();
    void computeAndSendHash();

    void finishPsi();
    void saveResults();

    void onSenderException(const std::string& _module, const std::exception& e);
    void onSenderTaskDone(bcos::Error::Ptr _error);

    void increaseOprfRound() { m_oprfRound++; }

    tools::Progress::Ptr progress() { return m_progress; }
    tools::Progress::Ptr matrixProgress() { return m_matrixProgress; }

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

    // width of per round processing
    uint32_t m_handleWidth;

    // size of each bucket (bytes)
    // the bucket size is also the height of matrix
    uint32_t m_bucketSizeInBytes;
    uint32_t m_mask;

    crypto::BitVector::Ptr m_otChoices;
    std::vector<bcos::bytes> m_receiverKeys;

    std::vector<std::array<uint32_t, 8>> m_originLocations;
    std::vector<std::vector<bcos::byte>> m_oprfOutputs;

    front::PPCChannelManager::Ptr m_channelManager;
    front::Channel::Ptr m_channel;

    std::vector<bcos::bytes> m_matrixC;

    std::shared_ptr<std::vector<uint8_t>> m_sResults;
    std::atomic<bool> m_resultCountReceived{false};
    uint32_t m_resultCount{0};

    uint32_t m_oprfRound{0};

    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimePoint;

    std::vector<long long> m_costs;

    tools::Progress::Ptr m_progress;
    tools::Progress::Ptr m_matrixProgress;
};
}  // namespace ppc::psi
