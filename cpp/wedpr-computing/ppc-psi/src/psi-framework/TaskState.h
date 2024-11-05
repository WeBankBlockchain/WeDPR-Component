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

    virtual ~TaskState() {}

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

    io::DataBatch::Ptr loadAllData()
    {
        io::DataBatch::Ptr results;
        auto originData = m_task->selfParty()->dataResource()->rawData();
        if (!originData.empty())
        {
            results = std::make_shared<ppc::io::DataBatch>();
            results->setDataSchema(ppc::io::DataSchema::Bytes);

            std::vector<bcos::bytes> data;
            for (auto& line : originData[0])
            {
                data.emplace_back(bcos::bytes(line.begin(), line.end()));
            }
            results->setData<bcos::bytes>(std::move(data));
        }
        else
        {
            if (m_reader)
            {
                int64_t nextParam = m_sqlReader ? 0 : -1;
                results = m_reader->next(nextParam, io::DataSchema::Bytes);
            }
        }
        if (!results)
        {
            results = std::make_shared<ppc::io::DataBatch>();
        }
        return results;
    }

    void writeLines(const io::DataBatch::Ptr& _data, io::DataSchema _schema)
    {
        if (m_writer)
        {
            m_writer->writeLine(_data, _schema);
            m_writer->flush();
            m_writer->close();
            m_writer->upload();
            m_uploaded = true;
        }
    }

    void writeBytes(bcos::bytesConstRef _data)
    {
        if (m_writer)
        {
            m_writer->writeBytes(_data);
            m_writer->flush();
            m_writer->close();
            m_writer->upload();
            m_uploaded = true;
        }
    }

    int32_t allocateSeq()
    {
        m_currentSeq.store(m_currentSeq.load() + 1);
        {
            bcos::WriteGuard l(x_seqList);
            m_seqList.insert(m_currentSeq.load());
        }
        return m_currentSeq;
    }

    void eraseFinishedTaskSeq(uint32_t _seq, bool _success)
    {
        {
            bcos::UpgradableGuard l(x_seqList);
            auto it = m_seqList.find(_seq);
            if (it == m_seqList.end())
            {
                return;
            }
            bcos::UpgradeGuard ul(l);
            m_seqList.erase(it);
            if (_success)
            {
                m_successCount++;
            }
            else
            {
                m_failedCount++;
            }
        }
        try
        {
            // trigger the callback when the sub-task finished
            // Note: the subTaskHandler may go wrong
            if (m_onSubTaskFinished)
            {
                m_onSubTaskFinished();
            }
            PSI_LOG(INFO) << LOG_DESC("eraseFinishedTaskSeq") << LOG_KV("task", m_task->id())
                          << LOG_KV("seq", _seq) << LOG_KV("success", _success)
                          << LOG_KV("seqs", m_seqList.size())
                          << LOG_KV("successCount", m_successCount)
                          << LOG_KV("failedCount", m_failedCount);
        }
        catch (std::exception const& e)
        {
            PSI_LOG(WARNING) << LOG_DESC(
                                    "eraseFinishedTaskSeq error for calls the sub-task-finalize "
                                    "handler exception")
                             << LOG_KV("msg", boost::diagnostic_information(e));
            onTaskException(boost::diagnostic_information(e));
        }
    }

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
    virtual void onTaskFinished()
    {
        // avoid repeated calls
        if (m_taskDone.exchange(true))
        {
            return;
        }
        PSI_LOG(INFO) << LOG_DESC("onTaskFinished") << LOG_KV("task", m_task->id())
                      << LOG_KV("success", m_successCount) << LOG_KV("failed", m_failedCount)
                      << LOG_KV("loadFinished", m_finished.load())
                      << LOG_KV("callback", m_callback ? "withCallback" : "emptyCallback");
        auto result = std::make_shared<ppc::protocol::TaskResult>(m_task->id());
        try
        {
            // upload the psi-result
            if (m_writer && !m_uploaded)
            {
                m_writer->upload();
                m_uploaded = true;
            }
            if (m_finalizeHandler)
            {
                m_finalizeHandler();
            }
            if (m_failedCount > 0)
            {
                auto error = std::make_shared<bcos::Error>(
                    -1, "task " + m_task->id() + " failed for " +
                            boost::lexical_cast<std::string>(m_failedCount) + " error!");
                result->setError(std::move(error));
                result->setStatus(ppc::protocol::toString(ppc::protocol::TaskStatus::FAILED));
            }
            else
            {
                result->setStatus(ppc::protocol::toString(ppc::protocol::TaskStatus::COMPLETED));
            }

            // clear file
            if (m_reader)
            {
                m_reader->clean();
            }
        }
        catch (std::exception const& e)
        {
            PSI_LOG(WARNING) << LOG_DESC("onTaskFinished exception")
                             << LOG_KV("msg", boost::diagnostic_information(e));
            auto error = std::make_shared<bcos::Error>(-1, boost::diagnostic_information(e));
            result->setError(std::move(error));
            result->setStatus(ppc::protocol::toString(ppc::protocol::TaskStatus::FAILED));
        }
        if (m_callback)
        {
            m_callback(std::move(result));
        }
    }

    virtual void onTaskFinished(ppc::protocol::TaskResult::Ptr _result, bool _noticePeer)
    {
        // avoid repeated calls
        if (m_taskDone.exchange(true))
        {
            return;
        }
        PSI_LOG(INFO) << LOG_DESC("onTaskFinished") << LOG_KV("task", m_task->id())
                      << LOG_KV("success", m_successCount) << LOG_KV("onlySelfRun", m_onlySelfRun)
                      << LOG_KV("finished", m_finished.load()) << LOG_KV("noticePeer", _noticePeer);
        if (!_result)
        {
            _result = std::make_shared<ppc::protocol::TaskResult>(m_task->id());
        }
        try
        {
            _result->setStatus(ppc::protocol::toString(ppc::protocol::TaskStatus::COMPLETED));
            if (_result->error() && _result->error()->errorCode() != 0)
            {
                _result->setStatus(ppc::protocol::toString(ppc::protocol::TaskStatus::FAILED));
            }
            // Note: we consider that the task success even if the handler exception
            if (_noticePeer && !m_onlySelfRun && _result->error() &&
                _result->error()->errorCode() && m_notifyPeerFinishHandler)
            {
                m_notifyPeerFinishHandler();
            }

            if (m_finalizeHandler)
            {
                m_finalizeHandler();
            }

            m_finished.exchange(true);

            // clear file
            if (m_reader)
            {
                m_reader->clean();
            }
        }
        catch (std::exception const& e)
        {
            PSI_LOG(WARNING) << LOG_DESC("onTaskFinished exception")
                             << LOG_KV("msg", boost::diagnostic_information(e));
            auto error = std::make_shared<bcos::Error>(-1, boost::diagnostic_information(e));
            _result->setError(std::move(error));
            _result->setStatus(ppc::protocol::toString(ppc::protocol::TaskStatus::FAILED));
        }
        if (m_callback)
        {
            m_callback(std::move(_result));
        }
    }

    virtual void onPeerNotifyFinish()
    {
        PSI_LOG(WARNING) << LOG_BADGE("onReceivePeerError") << LOG_KV("taskID", m_task->id());
        auto tesult = std::make_shared<protocol::TaskResult>(task()->id());
        tesult->setError(std::make_shared<bcos::Error>(
            (int)PSIRetCode::PeerNotifyFinish, "job participant sent an error"));
        onTaskFinished(std::move(tesult), false);
    }

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
        std::vector<bcos::bytes> const& _data)
    {
        bcos::RecursiveGuard l(m_mutex);
        // try to generate-default-output-desc to make sure the server output exists even if not
        // specified
        tryToGenerateDefaultOutputDesc();
        auto dataResource = m_task->selfParty()->dataResource();
        // load the writer
        if (!m_writer)
        {
            m_writer = _resourceLoader->loadWriter(dataResource->outputDesc());
        }
        auto dataBatch = std::make_shared<ppc::io::DataBatch>();
        dataBatch->setData(_data);
        m_writer->writeLine(dataBatch, ppc::io::DataSchema::Bytes);
        m_writer->flush();
    }

    std::function<void()> takeFinalizeHandler() { return std::move(m_finalizeHandler); }

    void onTaskException(std::string const& _errorMsg)
    {
        // set the task finished
        setFinished(true);
        {
            bcos::WriteGuard l(x_seqList);
            m_seqList.clear();
        }
        if (!m_callback)
        {
            return;
        }
        auto taskResult = std::make_shared<ppc::protocol::TaskResult>(m_task->id());
        auto msg = "Task " + m_task->id() + " exception, error : " + _errorMsg;
        auto error = std::make_shared<bcos::Error>(-1, msg);
        taskResult->setError(std::move(error));
        m_callback(std::move(taskResult));
        PSI_LOG(WARNING) << LOG_DESC(msg);
    }

    bool loadFinished() const { return m_finished.load(); }

    // generate default output-desc for given task
    void tryToGenerateDefaultOutputDesc()
    {
        auto dataResource = m_task->selfParty()->mutableDataResource();
        if (!dataResource)
        {
            dataResource = std::make_shared<ppc::protocol::DataResource>();
            m_task->mutableSelfParty()->setDataResource(dataResource);
        }
        if (dataResource->outputDesc())
        {
            return;
        }
        auto outputDesc = std::make_shared<ppc::protocol::DataResourceDesc>();
        auto dstPath = c_resultPath + "/" + m_task->id() + ".result";
        outputDesc->setPath(dstPath);
        outputDesc->setType((uint16_t)(ppc::protocol::DataResourceType::FILE));
        dataResource->setOutputDesc(outputDesc);
        PSI_LOG(INFO) << LOG_DESC("GenerateDefaultOutputDesc for the output-desc not specified")
                      << LOG_KV("task", m_task->id()) << LOG_KV("path", dstPath);
    }

    uint64_t taskPendingTime() { return (bcos::utcSteadyTime() - m_taskStartTime); }

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
