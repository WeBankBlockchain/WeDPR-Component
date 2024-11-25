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
 * @file CM2020PSIImpl.h
 * @author: shawnhe
 * @date 2022-12-7
 */

#pragma once
#include <bcos-utilities/CallbackCollectionHandler.h>
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/Worker.h>
#include <memory>
#include <queue>

#include "../psi-framework/TaskGuarder.h"
#include "../psi-framework/TaskState.h"
#include "CM2020PSIConfig.h"
#include "Common.h"
#include "core/CM2020PSIReceiver.h"
#include "core/CM2020PSISender.h"
#include "ppc-crypto/src/randomot/SimplestOT.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-front/ppc-front/PPCChannel.h"
#include "ppc-tools/src/common/TransTools.h"
#include "protocol/src/PPCMessage.h"

namespace ppc::psi
{
class CM2020PSIImpl : public bcos::Worker,
                      public TaskGuarder,
                      public ppc::task::TaskFrameworkInterface,
                      public std::enable_shared_from_this<CM2020PSIImpl>
{
public:
    using Ptr = std::shared_ptr<CM2020PSIImpl>;
    using CM2020PSIMsgQueue = bcos::ConcurrentQueue<front::PPCMessageFace::Ptr>;
    using CM2020PSIMsgQueuePtr = std::shared_ptr<CM2020PSIMsgQueue>;

    CM2020PSIImpl(const CM2020PSIConfig::Ptr& _config, unsigned _idleTimeMs = 0);

    ~CM2020PSIImpl() override = default;

    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    // register to the front to get the message related to cm2020-psi
    void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _message) override;

    void start() override;
    void stop() override;
    void onReceivedErrorNotification(ppc::front::PPCMessageFace::Ptr const& _message) override;
    void onSelfError(
        const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer) override;

    // cm2020-psi main processing function
    // for ut to make this function public
    void executeWorker() override;

protected:
    void asyncRunTask();
    void checkFinishedTask();

    void handleReceivedMessage(const ppc::front::PPCMessageFace::Ptr& _message);

    void onReceiveHelloReceiver(const ppc::front::PPCMessageFace::Ptr& _message);
    void onReceiveHelloSender(const ppc::front::PPCMessageFace::Ptr& _message);

    void onReceiveReceiverSize(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveSenderSize(ppc::front::PPCMessageFace::Ptr _message);

    void onReceivePointA(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveBatchPointB(ppc::front::PPCMessageFace::Ptr _message);

    void onReceiveMatrix(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveDoNextRound(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveHashes(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveResultCount(ppc::front::PPCMessageFace::Ptr _message);
    void onReceiveResults(ppc::front::PPCMessageFace::Ptr _message);

    void addTask(
        ppc::protocol::Task::ConstPtr _task, ppc::task::TaskResponseCallback&& _onTaskFinished)
    {
        bcos::WriteGuard l(x_taskQueue);
        m_taskQueue.push({std::move(_task), std::move(_onTaskFinished)});
    }

    CM2020PSIReceiver::Ptr findReceiver(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_receivers);
        auto it = m_receivers.find(_taskID);
        if (it != m_receivers.end())
        {
            return it->second;
        }
        return nullptr;
    }
    void addReceiver(CM2020PSIReceiver::Ptr _receiver)
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

    CM2020PSISender::Ptr findSender(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_senders);
        auto it = m_senders.find(_taskID);
        if (it != m_senders.end())
        {
            return it->second;
        }
        return nullptr;
    }
    void addSender(CM2020PSISender::Ptr _sender)
    {
        bcos::WriteGuard l(x_senders);
        m_senders[_sender->taskID()] = _sender;
    }
    void removeSender(const std::string& _taskID)
    {
        bcos::WriteGuard l(x_senders);
        auto it = m_senders.find(_taskID);
        if (it != m_senders.end())
        {
            m_senders.erase(it);
        }
    }

private:
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signal);
        m_signal.wait_for(l, boost::chrono::milliseconds(5));
    }

    void wakeupWorker() { m_signal.notify_all(); }

    CM2020PSIConfig::Ptr m_config;
    CM2020PSIMsgQueuePtr m_msgQueue;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    std::shared_ptr<bcos::ThreadPool> m_threadPool;
    crypto::SimplestOT::Ptr m_ot;

    std::shared_ptr<std::thread> m_thread;

    std::atomic<int> m_parallelism;
    std::queue<std::pair<ppc::protocol::Task::ConstPtr, ppc::task::TaskResponseCallback> >
        m_taskQueue;
    mutable bcos::SharedMutex x_taskQueue;

    std::unordered_map<std::string, CM2020PSIReceiver::Ptr> m_receivers;
    mutable bcos::SharedMutex x_receivers;

    std::unordered_map<std::string, CM2020PSISender::Ptr> m_senders;
    mutable bcos::SharedMutex x_senders;

    bool m_started{false};
    boost::condition_variable m_signal;
    boost::mutex x_signal;

    const int c_popWaitMs = 5;
};
}  // namespace ppc::psi