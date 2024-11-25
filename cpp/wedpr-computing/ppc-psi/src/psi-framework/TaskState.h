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
 * @file TaskState.h
 * @author: yujiechen
 * @date 2022-11-11
 */
#pragma once
#include "../Common.h"
#include "../PSIConfig.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/io/LineReader.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include <bcos-utilities/Common.h>
#include <boost/lexical_cast.hpp>
#include <atomic>
#include <memory>
#include <set>

namespace ppc::psi
{
class TaskState : public std::enable_shared_from_this<TaskState>
{
public:
    using Ptr = std::shared_ptr<TaskState>;
    TaskState(ppc::protocol::Task::ConstPtr const& _task,
        ppc::task::TaskResponseCallback&& _callback, bool _onlySelfRun = false,
        PSIConfig::Ptr _config = nullptr)
      : m_task(_task),
        m_callback(std::move(_callback)),
        m_onlySelfRun(_onlySelfRun),
        m_config(std::move(_config))
    {
        m_taskStartTime = bcos::utcSteadyTime();
    }

    virtual ~TaskState()
    {
        PSI_LOG(INFO) << LOG_DESC("** TaskState Destructor") << LOG_KV("task", m_task->id());
    }

    ppc::task::TaskResponseCallback const& callback() { return m_callback; }
    ppc::task::TaskResponseCallback takeCallback() { return std::move(m_callback); }
    bool onlySelfRun() { return m_onlySelfRun; }
    void setReader(io::LineReader::Ptr _reader, int64_t _readerParam)
    {
        m_reader = std::move(_reader);
        m_readerParam = _readerParam;
        if (m_reader)
        {
            m_sqlReader = (m_reader->type() == protocol::DataResourceType::MySQL);
        }
    }
    void setWriter(io::LineWriter::Ptr _writer) { m_writer = std::move(_writer); }
    protocol::Task::ConstPtr const& task() const { return m_task; }

    io::LineReader::Ptr const& reader() const { return m_reader; }
    io::LineWriter::Ptr const& writer() const { return m_writer; }

    int64_t readerParam() const { return m_readerParam; }
    bool sqlReader() const { return m_sqlReader; }

    io::DataBatch::Ptr loadAllData();
    void writeLines(const io::DataBatch::Ptr& _data, io::DataSchema _schema);
    void writeBytes(bcos::bytesConstRef _data);

    int32_t allocateSeq();

    void eraseFinishedTaskSeq(uint32_t _seq, bool _success);

    virtual void setFinished(bool _finished) { m_finished.store(_finished); }

    virtual std::string peerID() const { return m_peerID; }
    virtual void setPeerID(std::string const& _peerID) { m_peerID = _peerID; }

    virtual bool taskDone() { return m_taskDone; }

    // represent that the task has been finished or not
    virtual bool finished() const
    {
        bcos::ReadGuard l(x_seqList);
        return m_finished.load() && m_seqList.empty();
    }

    // trigger the callback to response
    virtual void onTaskFinished();

    virtual void onTaskFinished(ppc::protocol::TaskResult::Ptr _result, bool _noticePeer);

    virtual void onPeerNotifyFinish();
    void setWorker(std::function<void()> const& _worker) { m_worker = _worker; }
    void executeWork()
    {
        if (!m_worker)
        {
            return;
        }
        m_worker();
    }

    // set handler called when the task-finished
    // Note: the finalize-handler maybe called no matter task success or failed
    void registerFinalizeHandler(std::function<void()> const& _finalizeHandler)
    {
        m_finalizeHandler = _finalizeHandler;
    }

    void registerSubTaskFinishedHandler(std::function<void()> const& _onSubTaskFinished)
    {
        m_onSubTaskFinished = _onSubTaskFinished;
    }

    void registerNotifyPeerFinishHandler(std::function<void()> const& _notifyPeerFinishHandler)
    {
        m_notifyPeerFinishHandler = _notifyPeerFinishHandler;
    }

    // Note: must store the result serially
    void storePSIResult(ppc::io::DataResourceLoader::Ptr const& _resourceLoader,
        std::vector<bcos::bytes> const& _data);

    std::function<void()> takeFinalizeHandler() { return std::move(m_finalizeHandler); }

    void onTaskException(std::string const& _errorMsg);
    bool loadFinished() const { return m_finished.load(); }

    // generate default output-desc for given task
    void tryToGenerateDefaultOutputDesc();
    uint64_t taskPendingTime() { return (bcos::utcSteadyTime() - m_taskStartTime); }

    uint32_t sendedDataBatchSize() const { return m_seqList.size(); }

private:
    void removeGeneratedOutputFile();

protected:
    ppc::protocol::Task::ConstPtr m_task;
    ppc::task::TaskResponseCallback m_callback;
    bool m_onlySelfRun{false};
    PSIConfig::Ptr m_config;
    uint64_t m_taskStartTime = 0;

    // record the task-peer
    std::string m_peerID;

    std::function<void()> m_worker;
    // handler called after the task-finished
    std::function<void()> m_finalizeHandler;

    // handler called when the sub-task corresponding to the given seq completed
    std::function<void()> m_onSubTaskFinished;

    std::function<void()> m_notifyPeerFinishHandler;

    // the reader
    ppc::io::LineReader::Ptr m_reader;
    int64_t m_readerParam;
    bool m_sqlReader;
    io::LineWriter::Ptr m_writer = nullptr;

    // to load logic in segments, prevent memory from filling up
    // Note: only file has the segment logic
    std::atomic<uint32_t> m_currentSeq = {0};
    std::set<uint32_t> m_seqList;
    mutable bcos::SharedMutex x_seqList;

    uint64_t m_successCount = 0;
    uint64_t m_failedCount = 0;

    std::atomic<bool> m_taskDone{false};
    std::atomic<bool> m_finished = {false};

    mutable bcos::RecursiveMutex m_mutex;

    const std::string c_resultPath = "result";
    bool m_uploaded = false;
};

class TaskStateFactory
{
public:
    using Ptr = std::shared_ptr<TaskStateFactory>;
    TaskStateFactory() = default;
    virtual ~TaskStateFactory() = default;

    virtual TaskState::Ptr createTaskState(ppc::protocol::Task::ConstPtr const& _task,
        ppc::task::TaskResponseCallback&& _callback, bool _onlySelfRun = false,
        PSIConfig::Ptr _config = nullptr)
    {
        return std::make_shared<TaskState>(
            _task, std::move(_callback), _onlySelfRun, std::move(_config));
    }
};

}  // namespace ppc::psi
