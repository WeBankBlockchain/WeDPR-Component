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
 * @file TaskState.cpp
 * @author: yujiechen
 * @date 2022-11-11
 */
#include "TaskState.h"
using namespace ppc::psi;
using namespace ppc::io;
using namespace ppc::protocol;


DataBatch::Ptr TaskState::loadAllData()
{
    DataBatch::Ptr results;
    auto originData = m_task->selfParty()->dataResource()->rawData();
    if (!originData.empty())
    {
        results = std::make_shared<DataBatch>();
        results->setDataSchema(DataSchema::Bytes);

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
        results = std::make_shared<DataBatch>();
    }
    return results;
}

void TaskState::writeLines(const DataBatch::Ptr& _data, DataSchema _schema)
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

void TaskState::writeBytes(bcos::bytesConstRef _data)
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

int32_t TaskState::allocateSeq()
{
    bcos::RecursiveGuard l(m_mutex);
    m_currentSeq.store(m_currentSeq.load() + 1);
    {
        bcos::WriteGuard l(x_seqList);
        m_seqList.insert(m_currentSeq.load());
    }
    return m_currentSeq;
}

void TaskState::eraseFinishedTaskSeq(uint32_t _seq, bool _success)
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
                      << LOG_KV("seqs", m_seqList.size()) << LOG_KV("successCount", m_successCount)
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


// trigger the callback to response
void TaskState::onTaskFinished()
{
    // avoid repeated calls
    if (m_taskDone.exchange(true))
    {
        return;
    }
    PSI_LOG(INFO) << LOG_DESC("* onTaskFinished") << LOG_KV("task", m_task->id())
                  << LOG_KV("success", m_successCount) << LOG_KV("failed", m_failedCount)
                  << LOG_KV("loadFinished", m_finished.load())
                  << LOG_KV("callback", m_callback ? "withCallback" : "emptyCallback")
                  << LOG_KV("taskTimecost", taskPendingTime());
    auto result = std::make_shared<TaskResult>(m_task->id());
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
            result->setStatus(toString(TaskStatus::FAILED));
        }
        else
        {
            result->setStatus(toString(TaskStatus::COMPLETED));
        }

        // clear file
        if (m_reader)
        {
            m_reader->clean();
        }
    }
    catch (std::exception const& e)
    {
        PSI_LOG(WARNING) << LOG_DESC("* onTaskFinished exception")
                         << LOG_KV("taskTimeCost", taskPendingTime())
                         << LOG_KV("msg", boost::diagnostic_information(e));
        auto error = std::make_shared<bcos::Error>(-1, boost::diagnostic_information(e));
        result->setError(std::move(error));
        result->setStatus(toString(TaskStatus::FAILED));
    }
    if (m_callback)
    {
        m_callback(std::move(result));
    }
}

void TaskState::onTaskFinished(TaskResult::Ptr _result, bool _noticePeer)
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
        _result = std::make_shared<TaskResult>(m_task->id());
    }
    try
    {
        _result->setStatus(toString(TaskStatus::COMPLETED));
        if (_result->error() && _result->error()->errorCode() != 0)
        {
            _result->setStatus(toString(TaskStatus::FAILED));
        }
        // Note: we consider that the task success even if the handler exception
        if (_noticePeer && !m_onlySelfRun && _result->error() && _result->error()->errorCode() &&
            m_notifyPeerFinishHandler)
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
        _result->setStatus(toString(TaskStatus::FAILED));
    }
    if (m_callback)
    {
        m_callback(std::move(_result));
    }
}

void TaskState::onPeerNotifyFinish()
{
    PSI_LOG(WARNING) << LOG_BADGE("onReceivePeerError") << LOG_KV("taskID", m_task->id());
    auto tesult = std::make_shared<TaskResult>(task()->id());
    tesult->setError(std::make_shared<bcos::Error>(
        (int)PSIRetCode::PeerNotifyFinish, "job participant sent an error"));
    onTaskFinished(std::move(tesult), false);
}

// Note: must store the result serially
void TaskState::storePSIResult(
    DataResourceLoader::Ptr const& _resourceLoader, std::vector<bcos::bytes> const& _data)
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
    auto dataBatch = std::make_shared<DataBatch>();
    dataBatch->setData(_data);
    m_writer->writeLine(dataBatch, DataSchema::Bytes);
    m_writer->flush();
    PSI_LOG(INFO) << LOG_DESC("**** storePSIResult success ****") << LOG_KV("* task", m_task->id())
                  << LOG_KV("* IntersectionCount", _data.size())
                  << LOG_KV("* TaskTimecost", taskPendingTime());
}

void TaskState::onTaskException(std::string const& _errorMsg)
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
    // should been called even when exception
    if (m_finalizeHandler)
    {
        try
        {
            m_finalizeHandler();
        }
        catch (std::exception const& e)
        {
            PSI_LOG(WARNING) << LOG_DESC("finalize task exception")
                             << LOG_KV("taskID", m_task->id())
                             << LOG_KV("msg", boost::diagnostic_information(e));
        }
    }
    if (m_notifyPeerFinishHandler)
    {
        try
        {
            m_notifyPeerFinishHandler();
        }
        catch (std::exception const& e)
        {
            PSI_LOG(WARNING) << LOG_DESC("notifyPeerFinish exception")
                             << LOG_KV("taskID", m_task->id())
                             << LOG_KV("msg", boost::diagnostic_information(e));
        }
    }
    auto taskResult = std::make_shared<TaskResult>(m_task->id());
    auto msg = "Task " + m_task->id() + " exception, error : " + _errorMsg;
    auto error = std::make_shared<bcos::Error>(-1, msg);
    taskResult->setError(std::move(error));
    m_callback(std::move(taskResult));
    PSI_LOG(WARNING) << LOG_DESC(msg);
}

void TaskState::tryToGenerateDefaultOutputDesc()
{
    auto dataResource = m_task->selfParty()->mutableDataResource();
    if (!dataResource)
    {
        dataResource = std::make_shared<DataResource>();
        m_task->mutableSelfParty()->setDataResource(dataResource);
    }
    if (dataResource->outputDesc())
    {
        return;
    }
    auto outputDesc = std::make_shared<DataResourceDesc>();
    auto dstPath = c_resultPath + "/" + m_task->id() + ".result";
    outputDesc->setPath(dstPath);
    outputDesc->setType((uint16_t)(DataResourceType::FILE));
    dataResource->setOutputDesc(outputDesc);
    PSI_LOG(INFO) << LOG_DESC("GenerateDefaultOutputDesc for the output-desc not specified")
                  << LOG_KV("task", m_task->id()) << LOG_KV("path", dstPath);
}
