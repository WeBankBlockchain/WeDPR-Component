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
 * @file RA2018PSIImpl.cpp
 * @author: yujiechen
 * @date 2022-10-25
 */
#include "RA2018PSIImpl.h"
#include "core/CuckoofilterAllocator.h"
#include "core/Ra2018TaskParam.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/protocol/PPCMessageFace.h"

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace bcos;
using namespace ppc::front;
using namespace ppc::io;
using namespace ppc::task;

RA2018PSIImpl::RA2018PSIImpl(RA2018PSIConfig::Ptr const& _config,
    RA2018PSIStorage::Ptr const& _storage, unsigned _idleTimeMs, bool _waitResult, bool _disabled)
  : PSIFramework(_config->ra2018MsgFactory(), _config->dataResourceLoader(), _config, "ra2018psi",
        _idleTimeMs),
    m_disabled(_disabled),
    m_config(_config),
    m_storage(_storage),
    m_server(std::make_shared<RA2018PSIServer>(_config)),  // create the RA2018PSIServer to support
                                                           // fullEvaluate/Evaluate
    m_client(std::make_shared<RA2018PSIClient>(_config,
        _config->ra2018CacheCapacity())),  // create the RA2018PSIClient to support
                                           // blind/finalize/intersection, support multiple-tasks
    m_cuckooFiltersCache(std::make_shared<CuckooFiltersCache>(
        _storage, _config->cuckooFilterCacheSize())),  // the cuckoo-filters-cache
    m_worker(std::make_shared<bcos::ThreadPool>("offline-worker", 1)),
    m_waitResult(_waitResult)  // notify only when the task-finished or exceptioned
{
    m_notifier = m_client->onReady([&]() { wakeupWorker(); });
}

void RA2018PSIImpl::start()
{
    RA2018_LOG(INFO) << LOG_DESC("Start the RA2018PSI");
    // start a thread to execute task
    PSIFramework::start();
}

void RA2018PSIImpl::stop()
{
    RA2018_LOG(INFO) << LOG_DESC("Stop RA2018PSI");
    if (m_config->threadPool())
    {
        m_config->threadPool()->stop();
    }
    PSIFramework::stop();
    RA2018_LOG(INFO) << LOG_DESC("RA2018PSI stopped");
}

void RA2018PSIImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, TaskResponseCallback&& _onTaskFinished)
{
    if (m_disabled)
    {
        m_config->front()->notifyTaskInfo(_task->id());
        auto taskResult = std::make_shared<TaskResult>(_task->id());
        auto error = BCOS_ERROR_PTR(
            (int)RA2018PSIDisabled, "The ra2018-psi has been disabled by this node!");
        RA2018_LOG(WARNING) << LOG_DESC("response task result") << LOG_KV("task", _task)
                            << LOG_KV("code", error->errorCode())
                            << LOG_KV("msg", error->errorMessage());
        auto const& peerParties = _task->getAllPeerParties();

        auto notifiedError = BCOS_ERROR_PTR(
            (int)RA2018PSIDisabled, "The ra2018-psi has been disabled by the peer node!");
        for (auto const& it : peerParties)
        {
            notifyTaskResult(notifiedError, it.second->id(), _task->id(), "emptyResource");
        }
        taskResult->setError(std::move(error));
        _onTaskFinished(std::move(taskResult));
        // erase the task id
        m_config->front()->eraseTaskInfo(_task->id());
        return;
    }
    // parse the task command
    auto taskParam = std::make_shared<Ra2018TaskParam>(_task->param());
    auto dataResource = _task->selfParty()->dataResource();
    auto resourceID = dataResource ? dataResource->resourceID() : "empty";
    auto self = weak_from_this();
    auto taskState = m_taskStateFactory->createTaskState(
        _task, [self, _task, _onTaskFinished](TaskResult::Ptr&& _result) {
            // record the task time
            auto result = std::move(_result);
            _onTaskFinished(std::move(result));
            RA2018_LOG(INFO) << LOG_DESC("asyncRunTask finished") << printTaskInfo(_task);
        });
    auto const& taskID = _task->id();
    // Note: the finalizeHandler will be called no matter task success of failed
    taskState->registerFinalizeHandler([self, taskID]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        // erase the cache
        psi->m_client->eraseTaskCache(taskID);
        // erase task-info from the gateway
        psi->m_config->front()->eraseTaskInfo(taskID);
    });
    try
    {
        // Note: the psi-server no need to lock the resource and task
        auto error = lockResourceAndRecordTaskState(taskParam->command(), taskState);
        // response to the sdk
        if (error)
        {
            // trigger the callback
            auto taskResult = std::make_shared<TaskResult>(_task->id());
            RA2018_LOG(WARNING) << LOG_DESC("response task result") << LOG_KV("task", _task)
                                << LOG_KV("code", error->errorCode())
                                << LOG_KV("msg", error->errorMessage());
            taskResult->setError(std::move(error));
            _onTaskFinished(std::move(taskResult));
            return;
        }
        taskState->registerSubTaskFinishedHandler([self]() {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            psi->m_client->checkAndStoreInterSectionResult();
            psi->wakeupWorker();
        });

        switch (taskParam->command())
        {
        // data-preprocessing
        case (int)RA2018Command::DATA_PREPROCESSING:
        {
            asyncRunDataProcessing(
                _task->id(), dataResource, taskParam->operation(), _onTaskFinished);
            break;
        }
        case (int)RA2018Command::RUN_PSI:
        {
            runPSI(taskState);
            break;
        }
        default:
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)PSIRetCode::UnsupportedCommand,
                "UnsupportedCommand " + std::to_string(taskParam->command())));
            break;
        }
        }
    }
    catch (bcos::Error const& e)
    {
        RA2018_LOG(WARNING) << LOG_DESC("asyncRunTask exception, cancel the task")
                            << printTaskInfo(_task) << LOG_KV("code", e.errorCode())
                            << LOG_KV("msg", e.errorMessage());
        auto error = std::make_shared<Error>(e.errorCode(), e.errorMessage());
        onTaskError("asyncRunTask", std::move(error), taskState->peerID(), _task->id(), resourceID);
    }
    catch (std::exception const& e)
    {
        RA2018_LOG(WARNING) << LOG_DESC("asyncRunTask exception, cancel the task")
                            << printTaskInfo(_task)
                            << LOG_KV("msg", boost::diagnostic_information(e));
        auto error = std::make_shared<Error>(-1, boost::diagnostic_information(e));
        onTaskError("asyncRunTask", std::move(error), taskState->peerID(), _task->id(), resourceID);
    }
}

// run-psi
void RA2018PSIImpl::runPSI(TaskState::Ptr const& _taskState)
{
    auto task = _taskState->task();
    // notify the taskInfo to the front
    m_config->front()->notifyTaskInfo(task->id());
    switch (task->selfParty()->partyIndex())
    {
    case (int)ppc::protocol::PartyType::Client:
    {
        runClientPSI(_taskState);
        break;
    }
    // the server do-nothing
    case (int)ppc::protocol::PartyType::Server:
    {
        auto peerParty = checkAndSetPeerInfo(_taskState, false);
        if (!peerParty)
        {
            return;
        }
        // check the output resource existence when syncResultToPeer
        if (_taskState->task()->syncResultToPeer())
        {
            _taskState->tryToGenerateDefaultOutputDesc();
            m_config->dataResourceLoader()->checkResourceExists(
                task->selfParty()->dataResource()->outputDesc());
        }
        RA2018_LOG(INFO) << LOG_DESC("runServerPSI") << LOG_KV("peer", peerParty->id())
                         << printTaskInfo(task);
        break;
    }
    default:
    {
        auto error = std::make_shared<Error>((int)PSIRetCode::UnsupportedPartyType,
            "UnsupportedPartyType " + std::to_string(task->selfParty()->partyIndex()) +
                ", task: " + task->id());
        cancelTask(std::move(error), task->id());
        break;
    }
    }
}

void RA2018PSIImpl::asyncRunDataProcessing(std::string const& _taskID,
    DataResource::ConstPtr const& _dataResource, int _operation,
    TaskResponseCallback const& _callback)
{
    try
    {
        auto reader = loadData(m_config->dataResourceLoader(), _taskID, _dataResource);
        // Note: consider the offline-full-evaluate task takes-long-time, we resonse the result when
        // the task submit success
        if (!m_waitResult)
        {
            auto taskResult = std::make_shared<TaskResult>(_taskID);
            _callback(std::move(taskResult));
        }

        RA2018_LOG(INFO) << LOG_DESC("asyncRunDataProcessing") << LOG_KV("task", _taskID)
                         << printDataResourceInfo(_dataResource);
        // process offlineFullEvaluate
        auto startT = utcSteadyTime();
        // Note: we use one worker to process offline-evaluate task in case of it occupies
        // too-much cpu resource, and to release the rpc thread resource
        auto self = weak_from_this();
        m_worker->enqueue([self, startT, _taskID, _dataResource, _operation, reader,
                              _callback]() mutable {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            try
            {
                psi->offlineFullEvaluate(_dataResource->resourceID(), _operation, std::move(reader),
                    [self, startT, _dataResource, _taskID, _callback](Error::Ptr&& _error) {
                        if (_error)
                        {
                            RA2018_LOG(WARNING) << LOG_DESC("asyncRunDataProcessing error")
                                                << LOG_KV("task", _taskID)
                                                << LOG_KV("data", _dataResource->resourceID())
                                                << LOG_KV("code", _error->errorCode())
                                                << LOG_KV("msg", _error->errorMessage());
                        }
                        auto psi = self.lock();
                        if (!psi)
                        {
                            return;
                        }
                        // remove the dataResource from the processingDataResource pool, so that
                        // can accept the task related to the data-resource
                        psi->removeLockingResource(_dataResource->resourceID());
                        // erase the task record
                        psi->removePendingTask(_taskID);
                        if (psi->m_waitResult)
                        {
                            auto taskResult = std::make_shared<TaskResult>(_taskID);
                            _callback(std::move(taskResult));
                        }
                        RA2018_LOG(INFO)
                            << LOG_DESC("asyncRunDataProcessing success") << LOG_KV("task", _taskID)
                            << LOG_KV("timecost", (utcSteadyTime() - startT))
                            << printDataResourceInfo(_dataResource);
                    });
            }
            catch (std::exception const& e)
            {
                RA2018_LOG(WARNING)
                    << LOG_DESC("asyncRunDataProcessing exception")
                    << LOG_KV("error", boost::diagnostic_information(e)) << LOG_KV("task", _taskID)
                    << LOG_KV("timecost", (utcSteadyTime() - startT))
                    << printDataResourceInfo(_dataResource);
                auto error = BCOS_ERROR_PTR(-1, "asyncRunDataProcessing exception: " +
                                                    std::string(boost::diagnostic_information(e)));
                if (psi->m_waitResult)
                {
                    auto taskResult = std::make_shared<TaskResult>(_taskID);
                    taskResult->setError(error);
                    _callback(std::move(taskResult));
                }
                psi->cancelTask(std::move(error), _taskID);
            }
        });
    }
    catch (std::exception const& e)
    {
        RA2018_LOG(WARNING) << LOG_DESC("asyncRunDataProcessing exception")
                            << LOG_KV("error", boost::diagnostic_information(e));
        auto taskResult = std::make_shared<TaskResult>(_taskID);
        auto error =
            BCOS_ERROR_PTR(-1, "asyncRunDataProcessing: " + boost::diagnostic_information(e));
        taskResult->setError(error);
        _callback(std::move(taskResult));
        cancelTask(std::move(error), _taskID);
    }
}


void RA2018PSIImpl::offlineFullEvaluate(std::string const& _resourceID, int _operation,
    LineReader::Ptr _reader, std::function<void(Error::Ptr&&)> _callback)
{
    switch (_operation)
    {
    // offline data-insert
    case DataPreProcessingOption::Insert:
    {
        // only obtain the cuckoo-filter not full
        auto filterInfos = m_storage->getCuckooFilterInfos(_resourceID, true);
        evaluateAndInsert(_resourceID, std::move(_reader), std::move(filterInfos), _callback);
        break;
    }
    // offline data-delete
    case DataPreProcessingOption::Delete:
    {
        // get all the filters specified by _resourceID
        auto filterInfos = m_storage->getCuckooFilterInfos(_resourceID, false);
        evaluateAndDelete(_resourceID, std::move(_reader), std::move(filterInfos), _callback);
        break;
    }
    default:
    {
        _callback(BCOS_ERROR_PTR(
            -1, "Unsupported data pre-processing operation " + std::to_string(_operation)));
        break;
    }
    }
}

// delete the full-evaluated-data from the cuckoo-filter
void RA2018PSIImpl::evaluateAndDelete(std::string const& _resourceID, LineReader::Ptr _reader,
    CuckooFilterInfoSet&& _filterInfos, std::function<void(Error::Ptr&&)> _callback)
{
    auto sqlReader = (_reader->type() == ppc::protocol::DataResourceType::MySQL);
    DataBatch::Ptr data;
    if (sqlReader)
    {
        // load the first column data from the sql
        data = _reader->next(0, DataSchema::Bytes);
    }
    else
    {
        // load all data from the file(Note: we assume that the amount of data deleted each time is
        // not very large)
        data = _reader->next(-1, DataSchema::Bytes);
    }

    // the total filters expected to store
    auto totalStoreCount = std::make_shared<std::atomic<int>>(_filterInfos.size());
    auto callback = [totalStoreCount, _callback,
                        calledCount = std::make_shared<std::atomic<int>>(0),
                        failed = std::make_shared<std::atomic<int>>(0)](bcos::Error::Ptr _error) {
        (*calledCount)++;
        if (_error)
        {
            (*failed)++;
            RA2018_LOG(WARNING) << LOG_DESC("evaluateAndDelete: update cuckoo-filter error")
                                << LOG_KV("code", _error->errorCode())
                                << LOG_KV("msg", _error->errorMessage());
        }
        // not all store operations have been finished
        if (*calledCount != *totalStoreCount)
        {
            return;
        }
        // all the filters stored to backend successfully
        if ((*failed) == 0)
        {
            _callback(nullptr);
            return;
        }
        // with some-filters failed to store to the backend
        _callback(BCOS_ERROR_PTR(-1, "evaluateAndDelete failed for update cuckoo-filter error"));
    };

    //  evaluate the data
    auto result = m_server->fullEvaluate(data);
    // delete the result from all the filters
    // here load and update cuckoo-filter by batch to decrease the memory overhead
    int storeCount = 0;

    uint64_t i = 0;
    for (auto const& filterInfo : _filterInfos)
    {
        auto const& filterID = filterInfo->filterID();
        auto filter = m_storage->loadCuckooFilter(_resourceID, filterID);
        if (!filter)
        {
            i++;
            continue;
        }
        auto orgSize = result.size();
        // delete some elment from the cuckoo-filter
        filter->batchDeleteAndGetRemainKeys(result);
        // update the modified filter
        if (result.size() < orgSize)
        {
            storeCount++;
            // Note: the data has been deleted or the last-cuckoo-filter
            if (result.empty() || (i == (_filterInfos.size() - 1)))
            {
                // reset the totalStoreCount to the real-store-count
                // Note: must ensure the real totalStoreCount setted before calls
                // asyncUpdateCuckooFilter, otherwise the callback maybe never called
                *totalStoreCount = storeCount;
            }
            auto updatedFilter = std::make_shared<CuckooFilterInfo>(filterID, filterInfo->hash());
            updatedFilter->setCuckooFilter(std::move(filter));
            m_storage->asyncUpdateCuckooFilter(_resourceID, updatedFilter, callback);
        }
        // all data has been deleted, break
        if (result.empty())
        {
            break;
        }
        i++;
    }
    RA2018_LOG(INFO) << LOG_DESC("evaluateAndDelete finish") << LOG_KV("resource", _resourceID)
                     << LOG_KV("filterSize", _filterInfos.size())
                     << LOG_KV("storeCount", storeCount) << LOG_KV("failedCount", result.size());
}

// insert the full-evaluated-data into the cuckoo-filter
void RA2018PSIImpl::evaluateAndInsert(std::string const& _resourceID, LineReader::Ptr _reader,
    CuckooFilterInfoSet&& _reusableFilters, std::function<void(Error::Ptr&&)> _callback)
{
    auto sqlReader = (_reader->type() == ppc::protocol::DataResourceType::MySQL);
    // fileReader: load dataBatchSize() elements from the given file
    // sqlReader: load the first column
    auto nextParam = sqlReader ? 0 : m_config->cuckooFilterOption()->capacity;
    // the allocator to create a new cuckoo-filter or load the reusable cuckoo-filter from the
    // backend-db
    auto allocator = std::make_shared<CuckoofilterAllocator>(
        m_config, m_storage, _resourceID, _callback, std::move(_reusableFilters));
    RA2018_LOG(INFO) << LOG_DESC("evaluateAndInsert") << LOG_KV("resourceID", _resourceID)
                     << LOG_KV("nextParam", nextParam);
    // Note: should exit when stop
    do
    {
        auto data = _reader->next(nextParam, DataSchema::Bytes);
        // all data have been loaded
        if (!data)
        {
            break;
        }
        //  evaluate the data
        auto result = m_server->fullEvaluate(data);
        uint64_t offset = 0;
        // TODO: optimize here without copy
        std::set<bcos::bytes> resultSet(result.begin(), result.end());
        // insert and store all the result into cuckoo-filter
        do
        {
            // allocate cuckoo-filter to insert the result
            auto filter = allocator->allocate();
            // all elements has been inserted into the cuckoo-filter
            if (filter->batchInsert(resultSet))
            {
                break;
            }
        } while (true && m_started);
    } while (true && (!sqlReader) && m_started);  // Note: the sql only has one-column, the file
                                                  // support load by batch
    // flush all the memory cuckoo-filter into storage
    allocator->flush();
}

// sync the cuckoo-filter from the server
// Note: this operation can be enqueue into the threadPool to increase perf
void RA2018PSIImpl::syncCuckooFilter(
    std::string const& _peerID, std::string const& _taskID, DataResource::ConstPtr _dataResource)
{
    try
    {
        // load and init the cuckooFilterState
        auto state = m_client->loadCuckooFilterState(_dataResource->resourceID(), true);
        state->init();

        auto filterInfos = m_storage->getCuckooFilterInfos(_dataResource->resourceID(), false);
        RA2018_LOG(INFO) << LOG_DESC("syncCuckooFilter") << LOG_KV("task", _taskID)
                         << LOG_KV("peer", _peerID) << printDataResourceInfo(_dataResource)
                         << LOG_KV("filterInfos", filterInfos.size());
        requestCuckooFilters(_peerID, _taskID, _dataResource->resourceID(), filterInfos,
            (uint32_t)RA2018PacketType::CuckooFilterRequest);
    }
    catch (std::exception const& e)
    {
        std::stringstream ss;
        ss << LOG_DESC("runPSIClient: syncCuckooFilter exception") << LOG_KV("task", _taskID)
           << LOG_KV("peer", _peerID) << printDataResourceInfo(_dataResource)
           << LOG_KV("error", boost::diagnostic_information(e));
        auto errorMsg = ss.str();
        auto error = std::make_shared<Error>((int)PSIRetCode::syncCuckooFilterError, errorMsg);
        RA2018_LOG(WARNING) << errorMsg;
        // cancel the task
        onTaskError(
            "syncCuckooFilter", std::move(error), _peerID, _taskID, _dataResource->resourceID());
    }
}

// handle the cuckoo-filter response
void RA2018PSIImpl::handleCuckooFilterResponse(PSIMessageInterface::Ptr _msg)
{
    auto ra2018FilterMsg = std::dynamic_pointer_cast<RA2018FilterMessage>(_msg);
    RA2018_LOG(INFO) << LOG_DESC("handleCuckooFilterResponse") << printPSIMessage(ra2018FilterMsg)
                     << LOG_KV("cuckooFilterSize", ra2018FilterMsg->cuckooFilterSize());
    // Note: if the filter has not been changed, the filter will be nullptr
    auto const& filterInfo = ra2018FilterMsg->mutableFilterInfo();
    auto state = m_client->loadCuckooFilterState(ra2018FilterMsg->resourceID(), false);
    if (state)
    {
        state->setCuckooFilterSize(ra2018FilterMsg->cuckooFilterSize());
    }
    for (auto const& it : filterInfo)
    {
        handleCuckooFilterResponseItem(ra2018FilterMsg, state, it);
    }
    m_client->computeIntersection();
    // try to store the result
    m_client->checkAndStoreInterSectionResult();
}

// TODO: remove the erased cuckoo-filter of the client
bool RA2018PSIImpl::handleCuckooFilterResponseItem(RA2018FilterMessage::Ptr _msg,
    CuckooFilterState::Ptr _cuckooFilterState, CuckooFilterInfo::Ptr const& _cuckooFilterInfo)
{
    auto self = weak_from_this();
    auto onHandleFailed = [self, _msg, _cuckooFilterInfo](std::string const& _desc) {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        std::stringstream ss;
        ss << LOG_DESC("handleCuckooFilterResponse failed: ") << _desc
           << LOG_KV("task", _msg->taskID()) << LOG_KV("peer", _msg->partyID())
           << LOG_KV("resource", _msg->resourceID()) << _cuckooFilterInfo->desc();
        // not found the hitted cuckoo-filter
        // cancel the task
        auto errorMsg = ss.str();
        RA2018_LOG(WARNING) << errorMsg;
        auto error = BCOS_ERROR_PTR(PSIRetCode::LoadCuckooFilterDataError, std::move(errorMsg));

        psi->cancelTask(std::move(error), _msg->taskID());
    };
    RA2018_LOG(INFO) << LOG_DESC("handleCuckooFilterResponseItem: receive the cuckooFilter")
                     << printPSIMessage(_msg) << _cuckooFilterInfo->desc()
                     << LOG_KV("hit", _cuckooFilterInfo->cuckooFilter() ? false : true);
    if (!_cuckooFilterState)
    {
        onHandleFailed("the task cuckoo-filter resources are not prepared");
        return false;
    }
    auto cuckooFilterInfo = _cuckooFilterInfo;
    if (_cuckooFilterInfo->cuckooFilter())
    {
        // filter changed, load into the cache
        m_cuckooFiltersCache->insert(_msg->resourceID(), _cuckooFilterInfo);
    }
    else
    {
        // without change, load from local cuckoo-filter cache
        cuckooFilterInfo =
            m_cuckooFiltersCache->query(_msg->resourceID(), _cuckooFilterInfo->filterID());
        // request for the missed cuckoo-filter
        // Note: if the server miss the requested cuckoo-filters(the msg type is
        // MissingCuckooFilterResponse, no need to request again)
        if (!cuckooFilterInfo)
        {
            if (_msg->packetType() != (uint32_t)RA2018PacketType::MissingCuckooFilterResponse)
            {
                RA2018_LOG(INFO)
                    << LOG_DESC(
                           "Query filter from local storage failed, try to request the "
                           "cuckoo filter from the server")
                    << LOG_KV("resourceID", _msg->resourceID())
                    << LOG_KV("filterID", _cuckooFilterInfo->filterID()) << printPSIMessage(_msg);
                std::set<CuckooFilterInfo::Ptr> filters;
                filters.insert(_cuckooFilterInfo);
                // request the missed cuckoo-filter with given seq and cuckooFilterSize when
                // local-cuckoo-filter miss or broken
                requestCuckooFilters(_msg->from(), _msg->taskID(), _msg->resourceID(), filters,
                    (uint32_t)RA2018PacketType::MissingCuckooFilterRequest,
                    _msg->cuckooFilterSize(), _msg->seq());
            }
            else
            {
                RA2018_LOG(INFO) << LOG_DESC(
                                        "Query filter from server storage failed, set "
                                        "load-cuckoo-filter finished")
                                 << LOG_KV("resourceID", _msg->resourceID())
                                 << LOG_KV("filterID", _cuckooFilterInfo->filterID())
                                 << printPSIMessage(_msg);
                _cuckooFilterState->decreaseCuckooFilterSize(1);
            }
        }
    }
    if (cuckooFilterInfo)
    {
        _cuckooFilterState->appendCuckooFilterInfo(_msg->seq(), std::move(cuckooFilterInfo));
    }
    return true;
}

// the client run-psi
void RA2018PSIImpl::runClientPSI(TaskState::Ptr const& _taskState)
{
    // Note: enforce the peer-data-resource
    auto peerParty = checkAndSetPeerInfo(_taskState, true);
    if (!peerParty)
    {
        return;
    }
    auto const& peerID = peerParty->id();
    auto const& task = _taskState->task();
    auto const& dataResource = task->selfParty()->dataResource();
    RA2018_LOG(INFO) << LOG_DESC("runClientPSI") << printTaskInfo(task) << LOG_KV("peer", peerID);
    // check the dataResource
    if (!checkDataResourceForSelf(_taskState, peerID, true))
    {
        return;
    }
    if (!_taskState->task()->enableOutputExists())
    {
        // Note: if the output-resource already exists, will throw exception
        m_config->dataResourceLoader()->checkResourceExists(dataResource->outputDesc());
    }
    else
    {
        m_config->dataResourceLoader()->loadWriter(dataResource->outputDesc(), true);
    }
    // load the data
    auto reader = loadData(m_config->dataResourceLoader(), task->id(), dataResource);
    auto sqlReader = (reader->type() == ppc::protocol::DataResourceType::MySQL);
    // fileReader: load dataBatchSize() elements from the given file
    // sqlReader: load the first column
    auto nextParam = sqlReader ? 0 : m_config->dataBatchSize();
    _taskState->setReader(std::move(reader), nextParam);

    // sync the cuckooFilter
    auto self = weak_from_this();
    m_config->threadPool()->enqueue([self, peerID, task, peerParty]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        psi->syncCuckooFilter(peerID, task->id(), peerParty->dataResource());
    });
    // blind data
    auto cuckooFilterResourceID = peerParty->dataResource()->resourceID();
    blindData(cuckooFilterResourceID, _taskState);

    if (!_taskState->sqlReader())
    {
        // since one call to blindData may not handle all data of a task for cache limit
        // add the blindData to taskState worker to execute looply
        auto weakTaskState = std::weak_ptr<TaskState>(_taskState);
        _taskState->setWorker([weakTaskState, cuckooFilterResourceID, self]() {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            auto state = weakTaskState.lock();
            if (!state)
            {
                return;
            }
            psi->blindData(cuckooFilterResourceID, state);
        });
    }
}

// load data from DataResource and blind the plainData into cipher
// Note: the thread-pool executed function should not throw exception
bool RA2018PSIImpl::blindData(
    std::string const& _cuckooFilterResourceID, TaskState::Ptr const& _taskState)
{
    if (m_client->isFull())
    {
        return false;
    }
    auto dataResource = _taskState->task()->selfParty()->dataResource();
    auto self = weak_from_this();
    auto sendCallback = [self, _taskState, dataResource](Error::Ptr&& _error) {
        if (!_error || _error->errorCode() == 0)
        {
            return;
        }
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        RA2018_LOG(WARNING) << LOG_DESC("send evaluate request error")
                            << LOG_KV("peer", _taskState->peerID())
                            << printTaskInfo(_taskState->task())
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage());
        psi->cancelTask(std::move(_error), _taskState->task()->id());
    };
    // put the task into the thread-pool
    m_config->threadPool()->enqueue([self, _cuckooFilterResourceID, _taskState, dataResource,
                                        sendCallback]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        try
        {
            auto task = psi->getTaskByID(_taskState->task()->id());
            // the task has already been canceled
            if (!task)
            {
                return;
            }
            if (_taskState->loadFinished())
            {
                return;
            }
            do
            {
                int32_t seq = 0;
                DataBatch::Ptr dataBatch = nullptr;
                // Note: the setFinished/allocateSeq must be atomic, because they are used to
                // judge whether the task is finished
                {
                    bcos::Guard l(psi->m_mutex);
                    // Note: next is not thread-safe
                    dataBatch =
                        _taskState->reader()->next(_taskState->readerParam(), DataSchema::Bytes);
                    if (!dataBatch)
                    {
                        _taskState->setFinished(true);
                        break;
                    }
                    seq = _taskState->allocateSeq();
                    if (_taskState->sqlReader())
                    {
                        _taskState->setFinished(true);
                    }
                }
                auto dataBatchSize = dataBatch->size();
                auto ret = psi->m_client->insertCache(_cuckooFilterResourceID,
                    _taskState->task()->id(), _taskState, dataResource, seq, std::move(dataBatch));
                // blind error for insert the cache failed(the cache already exists)
                if (!ret.first && ret.second != nullptr)
                {
                    auto errorMsg = "The task " + _taskState->task()->id() +
                                    " is in-processing! Cancel the repeated task!";
                    auto error = BCOS_ERROR_PTR((int)PSIRetCode::TaskInProcessing, errorMsg);
                    psi->onTaskError("blindData", std::move(error), _taskState->peerID(),
                        _taskState->task()->id(), _cuckooFilterResourceID);
                    break;
                }
                auto cacheItem = ret.second;
                RA2018_LOG(INFO) << LOG_DESC("blindData") << printTaskInfo(_taskState->task())
                                 << LOG_KV("seq", seq) << LOG_KV("dataSize", dataBatchSize)
                                 << LOG_KV("cacheCapacity", psi->m_client->capacity());
                // the cache is full
                if (!cacheItem)
                {
                    break;
                }
                // trigger blind operation
                auto blindedData = cacheItem->blind();
                // send the evaluate request
                auto msg = psi->m_config->ra2018MsgFactory()->createRA2018FilterMessage(
                    (uint32_t)RA2018PacketType::EvaluateRequest);
                RA2018_LOG(INFO) << LOG_DESC("EvaluateRequest") << printTaskInfo(_taskState->task())
                                 << LOG_KV("seq", seq) << LOG_KV("dataSize", blindedData.size());
                msg->setResourceID(dataResource->resourceID());
                msg->setData(std::move(blindedData));
                psi->m_config->generateAndSendPPCMessage(
                    _taskState->peerID(), _taskState->task()->id(), msg, sendCallback, seq);
            } while (!_taskState->sqlReader() && psi->m_started);
        }
        catch (std::exception const& e)
        {
            std::stringstream ss;
            ss << LOG_DESC("blindData exception") << printTaskInfo(_taskState->task())
               << LOG_KV("error", boost::diagnostic_information(e));
            auto errorMsg = ss.str();
            auto error = std::make_shared<Error>((int)PSIRetCode::BlindDataError, errorMsg);
            RA2018_LOG(WARNING) << errorMsg;
            // cancel the task
            psi->onTaskError("blindData", std::move(error), _taskState->peerID(),
                _taskState->task()->id(),
                _taskState->task()->selfParty()->dataResource()->resourceID());
        }
    });
    return true;
}

// handle the evaluate data responsed from the server, try to finalize and intersection
void RA2018PSIImpl::handleEvaluateResponse(PSIMessageInterface::Ptr _msg)
{
    auto evaluatedData = _msg->takeData();
    RA2018_LOG(INFO) << LOG_DESC("handleEvaluateResponse") << printPSIMessage(_msg)
                     << LOG_KV("evaluatedSize", evaluatedData.size());
    auto clientCache = m_client->cache(_msg->taskID(), _msg->seq());
    // the task has been canceled
    if (!clientCache)
    {
        return;
    }
    // finliaize the evaluated-data
    clientCache->finalize(evaluatedData);
    // try to intersection
    clientCache->computeIntersection();
    // try to store the result
    m_client->checkAndStoreInterSectionResult();
}

// the cuckoo-filter of the client has been missed or broken
void RA2018PSIImpl::handleMissedCuckooFilterRequest(PSIMessageInterface::Ptr _msg)
{
    RA2018_LOG(INFO) << LOG_DESC("handleMissedCuckooFilterRequest") << printPSIMessage(_msg);
    auto cuckooFilterRequestMsg = std::dynamic_pointer_cast<RA2018FilterMessage>(_msg);
    auto const& filterInfos = cuckooFilterRequestMsg->filterInfo();
    for (auto const& it : filterInfos)
    {
        auto filterData = m_storage->loadCuckooFilterData(_msg->resourceID(), it->filterID());
        RA2018_LOG(INFO)
            << LOG_DESC("handleMissedCuckooFilterRequest: load cuckoo-filter from the storage")
            << LOG_KV("resource", _msg->resourceID()) << it->desc()
            << LOG_KV("size", filterData.size());
        it->setCuckooFilterData(std::move(filterData));
        responseCuckooFilter((uint32_t)(RA2018PacketType::MissingCuckooFilterResponse), _msg, it,
            cuckooFilterRequestMsg->cuckooFilterSize(), _msg->seq());
    }
}

// receive cuckoo-filter request, response the delta-cuckoo-filter
void RA2018PSIImpl::handleCuckooFilterRequest(PSIMessageInterface::Ptr _msg)
{
    auto startT = utcSteadyTime();
    auto cuckooFilterRequestMsg = std::dynamic_pointer_cast<RA2018FilterMessage>(_msg);
    auto& filterInfo = cuckooFilterRequestMsg->mutableFilterInfo();
    // load the local cuckooFilterInfo
    auto localCuckooFilterInfo = m_storage->getCuckooFilterInfos(_msg->resourceID(), false);
    // server miss the cukoo-filter
    if (localCuckooFilterInfo.empty())
    {
        auto errorMessage =
            "the server's data has not been processed offline, the intersection cannot be "
            "performed at present! Please trigger the offline-data-process firstly!";
        RA2018_LOG(INFO) << LOG_DESC("handleCuckooFilterRequest:") << errorMessage
                         << printPSIMessage(_msg) << LOG_KV("clientFilterSize", filterInfo.size())
                         << LOG_KV("localCuckooFilter", localCuckooFilterInfo.size());
        auto error =
            std::make_shared<bcos::Error>((int)PSIRetCode::NotOfflineFullEvaluated, errorMessage);
        // cancel the task and notify the peer
        onTaskError("handleCuckooFilterRequest", std::move(error), _msg->from(), _msg->taskID(),
            _msg->resourceID());
        return;
    }
    RA2018_LOG(INFO) << LOG_DESC("handleCuckooFilterRequest") << printPSIMessage(_msg)
                     << LOG_KV("clientFilterSize", filterInfo.size())
                     << LOG_KV("localCuckooFilter", localCuckooFilterInfo.size());
    std::vector<CuckooFilterInfo::Ptr> responseCuckooFilters;
    for (auto const& filter : filterInfo)
    {
        auto it = localCuckooFilterInfo.find(filter);
        // the filter is not change
        if (it != localCuckooFilterInfo.end())
        {
            // put the entry into the repsonse-filter-list with empty cuckoo-filter, which
            // represented that the filter is not changed
            responseCuckooFilters.emplace_back(*it);
            // erase the hitted entry from the local
            localCuckooFilterInfo.erase(it);
        }
    }
    RA2018_LOG(INFO) << LOG_DESC(
                            "handleCuckooFilterRequest: load the missed filter from the storage")
                     << printPSIMessage(_msg)
                     << LOG_KV("hittedFilters", responseCuckooFilters.size())
                     << LOG_KV("missedFilters", localCuckooFilterInfo.size());
    for (auto const& it : localCuckooFilterInfo)
    {
        auto filterData = m_storage->loadCuckooFilterData(_msg->resourceID(), it->filterID());
        if (filterData.empty())
        {
            continue;
        }
        RA2018_LOG(INFO) << LOG_DESC(
                                "handleCuckooFilterRequest: load cuckoo-filter from the storage")
                         << LOG_KV("resource", _msg->resourceID()) << it->desc()
                         << LOG_KV("size", filterData.size());
        it->setCuckooFilterData(std::move(filterData));
        // the missed entry with cuckoo-filter-data
        responseCuckooFilters.emplace_back(it);
    }
    // response to client
    // Note: for the network-msg-size limit, we send the one-cuckoo-filter per-network-msg
    size_t seq = 1;
    for (uint64_t i = 0; i < responseCuckooFilters.size(); i++)
    {
        responseCuckooFilter((uint32_t)RA2018PacketType::CuckooFilterResponse, _msg,
            responseCuckooFilters.at(i), responseCuckooFilters.size(), seq);
        seq++;
    }
    RA2018_LOG(INFO) << LOG_DESC("handleCuckooFilterRequest success")
                     << LOG_KV("responsedCuckooFilters", responseCuckooFilters.size())
                     << LOG_KV("timecost", (utcSteadyTime() - startT)) << printPSIMessage(_msg);
}

void RA2018PSIImpl::responseCuckooFilter(uint32_t _responseType,
    PSIMessageInterface::Ptr const& _msg, CuckooFilterInfo::Ptr const& _filterInfo,
    uint64_t _filterInfoSize, size_t _seq)
{
    auto cuckooFilterResponseMsg =
        m_config->ra2018MsgFactory()->createRA2018FilterMessage(_responseType);
    cuckooFilterResponseMsg->appendFilterInfo(_filterInfo);
    cuckooFilterResponseMsg->setResourceID(_msg->resourceID());
    cuckooFilterResponseMsg->setPartyID(m_config->selfParty());
    cuckooFilterResponseMsg->setCuckooFilterSize(_filterInfoSize);
    RA2018_LOG(INFO) << LOG_DESC("handleCuckooFilterRequest: response cuckoo-filter")
                     << printPSIMessage(_msg) << LOG_KV("seq", _seq) << _filterInfo->desc();
    auto self = weak_from_this();
    m_config->generateAndSendPPCMessage(
        _msg->from(), _msg->taskID(), cuckooFilterResponseMsg,
        [self, _msg](bcos::Error::Ptr&& _error) {
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            // TODO: retry or cancel task
            psi->onTaskError("handleCuckooFilterRequest: response cuckoo-filters",
                std::move(_error), _msg->from(), _msg->taskID(), _msg->resourceID());
        },
        _seq);
}

void RA2018PSIImpl::handleEvaluateRequest(PSIMessageInterface::Ptr _msg)
{
    auto startT = utcSteadyTime();
    auto&& blindedData = _msg->takeData();
    RA2018_LOG(INFO) << LOG_DESC("handleEvaluateRequest") << printPSIMessage(_msg)
                     << LOG_KV("blindDataItem", blindedData.size());
    // evaluate the blindedData
    auto evaluatedData = m_server->evaluate(blindedData);
    // generate ra2018-message, response the evaluated-data to the client
    auto response = m_config->ra2018MsgFactory()->createPSIMessage(
        (uint32_t)RA2018PacketType::EvaluateResponse);
    response->setData(std::move(evaluatedData));
    response->setResourceID(_msg->resourceID());
    response->setPartyID(m_config->selfParty());
    auto self = weak_from_this();
    // Note: must specified the seq here
    m_config->generateAndSendPPCMessage(
        _msg->from(), _msg->taskID(), response,
        [self, _msg](bcos::Error::Ptr&& _error) {
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            psi->onTaskError("handleEvaluateRequest: response evaluated-data", std::move(_error),
                _msg->from(), _msg->taskID(), _msg->resourceID());
        },
        _msg->seq());
    RA2018_LOG(INFO) << LOG_DESC("handleEvaluateRequest success") << printPSIMessage(_msg)
                     << LOG_KV("timecost", (utcSteadyTime() - startT));
}

void RA2018PSIImpl::handlePSIMsg(PSIMessageInterface::Ptr _msg)
{
    if (m_disabled)
    {
        handlePSIFrameworkMsg(_msg);
        return;
    }
    // check the task status
    if (!checkPSIMsg(_msg))
    {
        return;
    }
    // prioritize lightweight messages
    if (handlePSIFrameworkMsg(_msg))
    {
        return;
    }
    auto self = weak_from_this();
    m_config->threadPool()->enqueue([self, _msg]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        try
        {
            switch (_msg->packetType())
            {
            case (uint32_t)RA2018PacketType::CuckooFilterRequest:
            {
                psi->handleCuckooFilterRequest(_msg);
                break;
            }
            case (uint32_t)RA2018PacketType::MissingCuckooFilterRequest:
            {
                psi->handleMissedCuckooFilterRequest(_msg);
                break;
            }
            case (uint32_t)RA2018PacketType::MissingCuckooFilterResponse:
            case (uint32_t)RA2018PacketType::CuckooFilterResponse:
            {
                psi->handleCuckooFilterResponse(_msg);
                break;
            }
            case (uint32_t)RA2018PacketType::EvaluateRequest:
            {
                psi->handleEvaluateRequest(_msg);
                break;
            }
            case (uint32_t)RA2018PacketType::EvaluateResponse:
            {
                psi->handleEvaluateResponse(_msg);
                break;
            }
            default:
            {
                RA2018_LOG(WARNING)
                    << LOG_DESC("Unsupported packetType ") << (int)_msg->packetType();
                break;
            }
            }
        }
        catch (std::exception const& e)
        {
            RA2018_LOG(WARNING) << LOG_DESC("handlePSIMsg exception")
                                << LOG_KV("packetType", _msg->packetType()) << printPSIMessage(_msg)
                                << LOG_KV("error", boost::diagnostic_information(e));
            auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandleTaskError,
                "Task " + _msg->taskID() + " error for " + boost::diagnostic_information(e));
            psi->onTaskError("handlePSIMsg", std::move(error), _msg->from(), _msg->taskID(),
                "emptyDataResource");
        }
    });
}