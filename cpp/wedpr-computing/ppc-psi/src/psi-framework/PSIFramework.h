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
 * @file PSIFramework.h
 * @author: yujiechen
 * @date 2022-12-26
 */
#pragma once
#include "../PSIConfig.h"
#include "TaskState.h"
#include "interfaces/PSIMessageFactory.h"
#include "interfaces/PSIMessageInterface.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/ConcurrentQueue.h>
#include <bcos-utilities/ThreadPool.h>
#include <bcos-utilities/Timer.h>
#include <bcos-utilities/Worker.h>

#define PSI_FRAMEWORK_LOG(LEVEL)        \
    BCOS_LOG(LEVEL) << LOG_BADGE("PSI") \
                    << LOG_BADGE((ppc::protocol::TaskAlgorithmType)m_psiConfig->algorithmType())
namespace ppc::psi
{
using PSIMsgQueue = bcos::ConcurrentQueue<PSIMessageInterface::Ptr>;
using PSIMsgQueuePtr = std::shared_ptr<PSIMsgQueue>;
class PSIFramework : public bcos::Worker, public ppc::task::TaskFrameworkInterface
{
public:
    using Ptr = std::shared_ptr<PSIFramework>;
    PSIFramework(PSIMessageFactory::Ptr const& _msgFactory,
        ppc::io::DataResourceLoader::Ptr const& _dataResourceLoader, PSIConfig::Ptr const& _config,
        std::string const& _workerName = "psi-base", unsigned _idleTimeMs = 0)
      : Worker(_workerName, _idleTimeMs),
        m_msgFactory(_msgFactory),
        m_dataResourceLoader(_dataResourceLoader),
        m_psiConfig(_config),
        m_taskStateFactory(std::make_shared<TaskStateFactory>()),
        m_msgQueue(std::make_shared<PSIMsgQueue>())
    {
        // Note: c_taskSyncTimerPeriod is inited after PSIFramework created, so the timer should
        // been created after construct PSIFramework
        m_taskSyncTimer = std::make_shared<bcos::Timer>(c_taskSyncTimerPeriod, "taskSyncTimer");
    }
    virtual ~PSIFramework() = default;

    void start() override;
    void stop() override;

    // psi main processing function
    // for ut to make this function public
    void executeWorker() override;

    // register to the front to get the message related to
    void onReceiveMessage(ppc::front::PPCMessageFace::Ptr _msg) override;

    uint64_t lockingResourceSize() const
    {
        bcos::ReadGuard l(x_processingDataResource);
        return m_processingDataResource.size();
    }

    uint64_t pendingTasksSize() const
    {
        bcos::ReadGuard l(x_pendingTasks);
        return m_pendingTasks.size();
    }

    inline void removePendingTask(std::string const& _taskID)
    {
        bcos::WriteGuard l(x_pendingTasks);
        m_pendingTasks.erase(_taskID);
    }

    TaskState::Ptr getTaskByID(std::string const& _taskID)
    {
        bcos::ReadGuard l(x_pendingTasks);
        auto it = m_pendingTasks.find(_taskID);
        if (it != m_pendingTasks.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool checkPSIMsg(PSIMessageInterface::Ptr const& _msg);

    bool checkDataResourceForSelf(
        TaskState::Ptr const& _taskState, std::string const& _peerID, bool _requireOutput = false);

    PSIConfig::Ptr const& psiConfig() const { return m_psiConfig; }

protected:
    // handle the psi message
    virtual void handlePSIMsg(PSIMessageInterface::Ptr _msg) = 0;
    // should lock the party resource or not
    virtual bool needLockResource(int _command, int _partyIndex) = 0;
    // receive the handshake response
    virtual void onHandshakeResponse(PSIMessageInterface::Ptr const& _msg) = 0;
    // receive the handshake request
    virtual void onHandshakeRequest(PSIMessageInterface::Ptr const& _msg) = 0;

    // handle the taskNotification and taskInfos related messages
    virtual bool handlePSIFrameworkMsg(PSIMessageInterface::Ptr _msg);

    virtual bcos::Error::Ptr lockResourceAndRecordTaskState(
        int _command, TaskState::Ptr const& _taskState);

    // notify error-result to the peer and cancel-task
    void onTaskError(std::string const& _desc, bcos::Error::Ptr&& _error,
        std::string const& _peerID, std::string const& _taskID, std::string const& _resourceID);
    // cancel the task and response to the user when error happens
    virtual void cancelTask(bcos::Error::Ptr&& _error, std::string const& _task);


    // handle the local task looply
    virtual void handleLocalTask();
    virtual void checkAndNotifyTaskResult();

    // sync the task information and erase the task already deleted from peer
    virtual void syncTaskInfo();
    virtual void handleTaskSyncInfo(PSIMessageInterface::Ptr _msg);

    // receive the task-cancel-msg
    virtual void handleTaskNotificationMsg(PSIMessageInterface::Ptr _msg);

    /////// the common function
    // notify the peer the task has been canceled for some error
    virtual void notifyTaskResult(bcos::Error::Ptr const& _error, std::string const& _peerID,
        std::string const& _taskID, std::string const& _resourceID);

    template <typename T>
    inline void batchRemoveDataResource(T const& _resourceList)
    {
        if (_resourceList.empty())
        {
            return;
        }
        bcos::WriteGuard l(x_processingDataResource);
        for (auto const& it : _resourceList)
        {
            m_processingDataResource.erase(it);
        }
    }

    virtual void removeLockingResource(std::string const& _resourceID)
    {
        // update the processing-data-resource
        bcos::UpgradableGuard l(x_processingDataResource);
        auto it = m_processingDataResource.find(_resourceID);
        if (it != m_processingDataResource.end())
        {
            bcos::UpgradeGuard ul(l);
            m_processingDataResource.erase(it);
        }
    }

    void wakeupWorker() { m_signalled.notify_all(); }

    virtual ppc::protocol::PartyResource::Ptr checkAndSetPeerInfo(
        TaskState::Ptr const& _taskState, bool _enforcePeerResource);
    ppc::io::LineReader::Ptr loadData(ppc::io::DataResourceLoader::Ptr _dataResourceLoader,
        std::string const& _taskID, ppc::protocol::DataResource::ConstPtr const& _dataResource);

    // the client send the handshakeRequest to the server
    virtual void sendHandshakeRequest(TaskState::Ptr const& _taskState);

    // handle the PSIResultSyncMsg
    virtual void handlePSIResultSyncMsg(PSIMessageInterface::Ptr _resultSyncMsg);

private:
    // utility functions
    void waitSignal()
    {
        boost::unique_lock<boost::mutex> l(x_signalled);
        m_signalled.wait_for(l, boost::chrono::milliseconds(5));
    }
    void responsePSIResultSyncStatus(int32_t _code, std::string const& _msg,
        bcos::bytes const& _peer, std::string const& _taskID, std::string const& _uuid,
        uint32_t _seq);

    void broadcastSyncTaskInfo(
        std::string const& _peerID, std::vector<std::string> const& _taskList);

    void updatePeerTasks(std::string const& _agencyID, std::string const& _partyID,
        std::set<std::string> const& _tasks)
    {
        bcos::WriteGuard l(x_peerTasks);
        m_peerTasks[_agencyID][_partyID] = _tasks;
    }

    std::map<std::string, std::set<std::string>> getPeerTasks(std::string const& _agencyID) const
    {
        bcos::ReadGuard l(x_peerTasks);
        if (m_peerTasks.count(_agencyID))
        {
            return m_peerTasks.at(_agencyID);
        }
        return std::map<std::string, std::set<std::string>>();
    }

protected:
    PSIMessageFactory::Ptr m_msgFactory;
    ppc::io::DataResourceLoader::Ptr m_dataResourceLoader;
    PSIConfig::Ptr m_psiConfig;
    // the task-state factory
    TaskStateFactory::Ptr m_taskStateFactory;

    bool m_started = false;

    boost::condition_variable m_signalled;
    boost::mutex x_signalled;

    mutable bcos::Mutex m_mutex;

    // the processing data resource
    std::set<std::string> m_processingDataResource;
    mutable bcos::SharedMutex x_processingDataResource;

    // record the pending tasks(Note: only psi task recorded here)
    std::unordered_map<std::string, TaskState::Ptr> m_pendingTasks;
    mutable bcos::SharedMutex x_pendingTasks;

    PSIMsgQueuePtr m_msgQueue;
    const unsigned c_PopWaitMs = 5;

    // the timer used to sync the task-information and evict the expired or exceptioned task
    // periodically
    std::shared_ptr<bcos::Timer> m_taskSyncTimer;
    const unsigned int c_taskSyncTimerPeriod = 10000;

    // record the tasks of the peer
    std::map<std::string, std::map<std::string, std::set<std::string>>> m_peerTasks;
    mutable bcos::SharedMutex x_peerTasks;
};
}  // namespace ppc::psi