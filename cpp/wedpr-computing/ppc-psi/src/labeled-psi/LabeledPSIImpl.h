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
 * @file LabeledPSIImpl.h
 * @author: shawnhe
 * @date 2022-11-7
 */

#pragma once
#include <apsi/psi_params.h>
#include <bcos-utilities/CallbackCollectionHandler.h>
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/Worker.h>
#include <memory>

#include "../psi-framework/TaskGuarder.h"
#include "Common.h"
#include "LabeledPSIConfig.h"
#include "core/LabeledPSIReceiver.h"
#include "core/LabeledPSISender.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-front/ppc-front/PPCChannel.h"
#include "ppc-tools/src/common/TransTools.h"
#include "protocol/src/PPCMessage.h"

namespace ppc::psi
{
class LabeledPSIImpl : public bcos::Worker,
                       public TaskGuarder,
                       public ppc::task::TaskFrameworkInterface,
                       public std::enable_shared_from_this<LabeledPSIImpl>
{
public:
    using Ptr = std::shared_ptr<LabeledPSIImpl>;
    using LabeledPSIMsgQueue = bcos::ConcurrentQueue<front::PPCMessageFace::Ptr>;
    using LabeledPSIMsgQueuePtr = std::shared_ptr<LabeledPSIMsgQueue>;

    LabeledPSIImpl(LabeledPSIConfig::Ptr _config, unsigned _idleTimeMs = 0);

    virtual ~LabeledPSIImpl() = default;

    // run task
    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    // register to the front to get the message related to labeled-psi
    void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _message) override;

    void start() override;
    void stop() override;

    void onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message) override;
    void onSelfError(
        const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer) override;

    // labeled-psi main processing function
    // for ut to make this function public
    void executeWorker() override;

protected:
    void asyncRunSenderTask(const ppc::protocol::Task::ConstPtr& _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished);

    void checkFinishedTask();

    void onSenderTaskDone(const std::string& _taskID);

    // init senderDB
    void setupSenderDB(const ppc::protocol::Task::ConstPtr& _task, int _labelByteCount);
    void saveSenderCache(const ppc::protocol::Task::ConstPtr& _task);
    void loadSenderCache(const ppc::protocol::Task::ConstPtr& _task);

    void handleReceivedMessage(const ppc::front::PPCMessageFace::Ptr& _message);
    void onReceivePsiParamsRequest(ppc::front::PPCMessageFace::Ptr _message);
    void onReceivePsiParamsResponse(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveBlindedItems(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveEvaluatedItems(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveQuery(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveResponse(ppc::front::PPCMessageFace::Ptr _message);

    LabeledPSIReceiver::Ptr findReceiver(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_receivers);
        auto it = m_receivers.find(_taskID);
        if (it != m_receivers.end())
        {
            return it->second;
        }
        return nullptr;
    }
    void addReceiver(LabeledPSIReceiver::Ptr _receiver)
    {
        bcos::WriteGuard l(x_receivers);
        m_receivers[_receiver->taskID()] = _receiver;
    }
    void removeReceiver(const std::string& _taskID)
    {
        bcos::WriteGuard l(x_receivers);
        auto it = m_receivers.find(_taskID);
        if (it != m_receivers.end())
        {
            m_receivers.erase(it);
        }
    }


private:
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signal);
        m_signal.wait_for(l, boost::chrono::milliseconds(5));
    }

    void wakeupWorker() { m_signal.notify_all(); }

    LabeledPSIConfig::Ptr m_config;
    LabeledPSIMsgQueuePtr m_msgQueue;
    bcos::ThreadPool::Ptr m_worker;

    LabeledPSISender::Ptr m_sender;
    std::atomic<bool> m_senderReady{false};

    std::unordered_map<std::string, LabeledPSIReceiver::Ptr> m_receivers;
    mutable bcos::SharedMutex x_receivers;

    SenderDB::Ptr m_senderDB;

    bool m_started = false;
    boost::condition_variable m_signal;
    boost::mutex x_signal;

    const int c_popWaitMs = 5;
};
}  // namespace ppc::psi