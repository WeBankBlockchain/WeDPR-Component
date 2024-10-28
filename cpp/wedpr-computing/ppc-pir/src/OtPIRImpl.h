/*
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
 * @file AYSService.h
 * @author: asherli
 * @date 2023-03-13
 */
#pragma once
#include "Common.h"
// #include "OtPIR.h"
#include "BaseOT.h"
#include "OtPIRConfig.h"
#include "ppc-framework/protocol/DataResource.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-psi/src/psi-framework/TaskGuarder.h"
#include "ppc-rpc/src/RpcFactory.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/Worker.h>
#include <map>
#include <string>

namespace ppc::pir
{
class OtPIRImpl : public std::enable_shared_from_this<OtPIRImpl>,
                  public bcos::Worker,
                  public psi::TaskGuarder,  // taskGuarder并不一定属于psi-framework
                  public ppc::task::TaskFrameworkInterface
// class OtPIRImpl : public std::enable_shared_from_this<OtPIRImpl>
{
public:
    using Ptr = std::shared_ptr<OtPIRImpl>;
    // OtPIRImpl() = default;
    OtPIRImpl(const OtPIRConfig::Ptr& _config, unsigned _idleTimeMs = 0);
    ~OtPIRImpl() override = default;
    using OtPIRMsgQueue = bcos::ConcurrentQueue<front::PPCMessageFace::Ptr>;
    using OtPIRMsgQueuePtr = std::shared_ptr<OtPIRMsgQueue>;

    // run task
    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    void start() override;
    void stop() override;

    // register to the front to get the message related to ot-pir
    void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _message) override;

    void onReceivedErrorNotification(const std::string& _taskID) override;
    void onSelfError(
        const std::string& _taskID, bcos::Error::Ptr _error, bool _noticePeer) override;

    // ot-pir main processing function
    // for ut to make this function public
    void executeWorker() override;

    void handleReceivedMessage(const ppc::front::PPCMessageFace::Ptr& _message);

    void onHelloReceiver(const ppc::front::PPCMessageFace::Ptr& _message);
    // void onReceiveHelloSender(const ppc::front::PPCMessageFace::Ptr& _message);

    void onSnederResults(ppc::front::PPCMessageFace::Ptr _message);
    const std::string& taskID() const { return m_taskID; }


    // void makeBaseOt(Json::Value const& request, Json::Value& response);
    // void encryptDataset(Json::Value const& request, Json::Value& response);

    // void setAYSConfig(AYSConfig const& aysConfig);
    // std::vector<bcos::bytes> prepareDataset(bcos::bytes sendObfuscatedHash, std::string
    // datasetPath);

protected:
    void asyncRunTask();
    void checkFinishedTask();
    void runSenderGenerateCipher(PirTaskMessage taskMessage);
    void saveResults(bcos::bytes result);

    void onReceiverTaskDone(bcos::Error::Ptr _error);
    // void runReceiverGenerateMessage(ppctars::SenderMessageParams senderMessageParams);
    // void runFinishSender(PirTaskMessage taskMessage);

    void addTask(
        ppc::protocol::Task::ConstPtr _task, ppc::task::TaskResponseCallback&& _onTaskFinished)
    {
        bcos::WriteGuard l(x_taskQueue);
        m_taskQueue.push({std::move(_task), std::move(_onTaskFinished)});
    }

    crypto::ReceiverMessage findReceiver(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_receivers);
        auto it = m_receivers.find(_taskID);
        if (it != m_receivers.end())
        {
            return it->second;
        }
        return crypto::ReceiverMessage();
    }
    void addReceiver(crypto::ReceiverMessage _receiver)
    {
        bcos::WriteGuard l(x_receivers);
        m_receivers[m_taskID] = _receiver;
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

    crypto::SenderMessage findSender(const std::string& _taskID)
    {
        bcos::ReadGuard l(x_senders);
        auto it = m_senders.find(_taskID);
        if (it != m_senders.end())
        {
            return it->second;
        }
        return crypto::SenderMessage();
    }
    void addSender(crypto::SenderMessage _sender)
    {
        bcos::WriteGuard l(x_senders);
        // m_taskID
        m_senders[m_taskID] = _sender;
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
    // allow the output-path exists, for ut
    bool m_enableOutputExists = false;
    // 为true时启动时会从配置中加载文件作为匹配源
    bool m_enableMemoryFile = false;
    ppc::protocol::DataResource m_resource;


private:
    void wakeupWorker() { m_signal.notify_all(); }

    bool m_started{false};
    std::shared_ptr<std::thread> m_thread;
    std::shared_ptr<boost::asio::io_service> m_ioService;
    OtPIRConfig::Ptr m_config;

    std::shared_ptr<bcos::ThreadPool> m_senderThreadPool;
    crypto::BaseOT::Ptr m_ot;
    std::atomic<int> m_parallelism;
    OtPIRMsgQueuePtr m_msgQueue;

    boost::condition_variable m_signal;

    const int c_popWaitMs = 5;
    boost::mutex x_signal;
    // 查询方需要的数据，包含要发送的和私有保存的
    // crypto::SenderMessage senderMessage;
    std::unordered_map<std::string, crypto::SenderMessage> m_senders;
    mutable bcos::SharedMutex x_senders;

    // 数据方发送的数据，全部要发送
    // crypto::ReceiverMessage receiverMessage;
    std::unordered_map<std::string, crypto::ReceiverMessage> m_receivers;
    mutable bcos::SharedMutex x_receivers;

    std::vector<std::pair<bcos::bytes, bcos::bytes>> messageKeypair;
    std::queue<std::pair<ppc::protocol::Task::ConstPtr, ppc::task::TaskResponseCallback>>
        m_taskQueue;
    mutable bcos::SharedMutex x_taskQueue;

    // // setup的系统参数，分桶大小来决定披露前k个bit 目前版本由TASK先传入
    // const int obfuscation_order = 6;
    std::string m_taskID;
    ppc::psi::TaskState::Ptr m_taskState;
    ppc::protocol::TaskResult::Ptr m_taskResult;

    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signal);
        m_signal.wait_for(l, boost::chrono::milliseconds(5));
    }
};
}  // namespace ppc::pir