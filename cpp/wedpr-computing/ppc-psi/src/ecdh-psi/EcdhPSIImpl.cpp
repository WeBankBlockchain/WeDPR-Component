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
 * @file EcdhPSIImpl.cpp
 * @author: yujiechen
 * @date 2022-12-27
 */
#include "EcdhPSIImpl.h"
#include "ppc-framework/protocol/GlobalConfig.h"

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::crypto;
using namespace ppc::io;
using namespace bcos;
using namespace ppc::task;

void EcdhPSIImpl::asyncRunTask(
    ppc::protocol::Task::ConstPtr _task, TaskResponseCallback&& _onTaskFinished)
{
    try
    {
        // check the partyIndex
        auto partyIndex = _task->selfParty()->partyIndex();
        if (partyIndex != (int)PartyType::Client && partyIndex != (int)PartyType::Server)
        {
            auto taskResult = std::make_shared<TaskResult>(_task->id());
            auto error = BCOS_ERROR_PTR((int)TaskParamsError,
                "The party index of the ecdh-psi must be client(0) or server(1)!");
            ECDH_LOG(WARNING) << LOG_DESC("response task result") << LOG_KV("task", _task)
                              << LOG_KV("code", error->errorCode())
                              << LOG_KV("msg", error->errorMessage());
            taskResult->setError(std::move(error));
            _onTaskFinished(std::move(taskResult));
            return;
        }
        // create the taskState
        auto self = weak_from_this();
        auto taskState = m_taskStateFactory->createTaskState(
            _task, [self, _task, _onTaskFinished](TaskResult::Ptr&& _result) {
                auto result = std::move(_result);
                _onTaskFinished(std::move(result));
                ECDH_LOG(INFO) << LOG_DESC("asyncRunTask finished") << printTaskInfo(_task);
            });
        auto const& taskID = _task->id();
        taskState->registerFinalizeHandler([self, taskID]() {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            // erase the cache
            psi->m_cache->eraseCache(taskID);
            // erase the taskInfo from the gateway
            psi->m_config->front()->eraseTaskInfo(taskID);
        });
        // Note: ecdh-psi with only run-psi command
        auto error = lockResourceAndRecordTaskState(0, taskState);
        if (error)
        {
            // trigger the callback
            auto taskResult = std::make_shared<TaskResult>(_task->id());
            ECDH_LOG(WARNING) << LOG_DESC("response task result") << LOG_KV("task", _task)
                              << LOG_KV("code", error->errorCode())
                              << LOG_KV("msg", error->errorMessage());
            taskResult->setError(std::move(error));
            _onTaskFinished(std::move(taskResult));
            return;
        }
        // set the peerID
        auto peerParty = checkAndSetPeerInfo(taskState, false);
        if (!peerParty)
        {
            return;
        }
        auto const& peerID = peerParty->id();
        // check the dataResource
        bool server = (partyIndex == (int)PartyType::Server) ? true : false;
        if (!checkDataResourceForSelf(taskState, peerID, !server))
        {
            return;
        }
        taskState->registerSubTaskFinishedHandler([self]() {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            // tryToIntersectionAndStoreResult will erase the finished task, update the capacity and
            // trigger next-round data-blinding
            psi->m_cache->tryToIntersectionAndStoreResult();
            psi->wakeupWorker();
        });
        auto ret = initTaskState(taskState);
        if (ret)
        {
            runPSI(taskState);
        }
    }
    catch (std::exception const& e)
    {
        ECDH_LOG(WARNING) << LOG_DESC("asyncRunTask exception, cancel the task")
                          << printTaskInfo(_task)
                          << LOG_KV("msg", boost::diagnostic_information(e));
        auto error = std::make_shared<Error>(
            -1, "asyncRunTask " + _task->id() + " exception: " + boost::diagnostic_information(e));
        cancelTask(std::move(error), _task->id());
    }
}

void EcdhPSIImpl::runPSI(TaskState::Ptr const& _taskState)
{
    ECDH_LOG(INFO) << LOG_DESC("runPSI") << printTaskInfo(_taskState->task());
    // notify the taskInfo to the front
    m_config->front()->notifyTaskInfo(_taskState->task()->id());
    // the psi-client send handshake request to the server
    if (_taskState->task()->selfParty()->partyIndex() == (int)PartyType::Client)
    {
        sendHandshakeRequest(_taskState);
    }
}

// handle the psi message
void EcdhPSIImpl::handlePSIMsg(PSIMessageInterface::Ptr _msg)
{
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
            case (uint32_t)ECDHPacketType::EvaluateRequest:
            {
                psi->handleEvaluateRequest(_msg);
                break;
            }
            case (uint32_t)ECDHPacketType::EvaluateResponse:
            {
                psi->handleEvaluateResponse(_msg);
                break;
            }
            case (uint32_t)ECDHPacketType::ServerBlindedData:
            {
                psi->handleServerBlindData(_msg);
                break;
            }
            case (uint32_t)ECDHPacketType::SyncDataBatchInfo:
            {
                psi->onRecvSyncDataInfoMsg(_msg);
                break;
            }
            default:
            {
                ECDH_LOG(ERROR) << LOG_DESC("Unsupported packetType ") << (int)_msg->packetType();
                break;
            }
            }
        }
        catch (std::exception const& e)
        {
            ECDH_LOG(ERROR) << LOG_DESC("handlePSIMsg exception")
                            << LOG_KV("packetType", _msg->packetType()) << printPSIMessage(_msg)
                            << LOG_KV("error", boost::diagnostic_information(e));

            auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandleTaskError,
                "Task " + _msg->taskID() + " error for " + boost::diagnostic_information(e));
            psi->onTaskError("handlePSIMsg", std::move(error), _msg->from(), _msg->taskID(),
                "emptyDataResource");
        }
    });
}

bool EcdhPSIImpl::initTaskState(TaskState::Ptr const& _taskState)
{
    auto task = _taskState->task();
    auto dataResource = task->selfParty()->dataResource();
    bool server = (task->selfParty()->partyIndex() == (int)PartyType::Server) ? true : false;
    try
    {
        // the client
        if (!server)
        {
            m_cache->insertServerCipherCache(task->id(), _taskState);
            if (!m_enableOutputExists)
            {
                // Note: if the output-resource already exists, will throw exception
                m_config->dataResourceLoader()->checkResourceExists(dataResource->outputDesc());
            }
            else
            {
                m_config->dataResourceLoader()->loadWriter(dataResource->outputDesc(), true);
            }
        }
        else if (_taskState->task()->syncResultToPeer())
        {
            // check the output resource existence when syncResultToPeer
            _taskState->tryToGenerateDefaultOutputDesc();
            m_config->dataResourceLoader()->checkResourceExists(
                task->selfParty()->dataResource()->outputDesc());
        }
        // load the data
        auto reader = loadData(m_config->dataResourceLoader(), task->id(), dataResource);
        // Note: the sql read all data at one time
        auto sqlReader = (reader->type() == ppc::protocol::DataResourceType::MySQL);
        // fileReader: load dataBatchSize() elements from the given file
        // sqlReader: load the first column
        auto nextParam = sqlReader ? 0 : m_config->dataBatchSize();
        _taskState->setReader(std::move(reader), nextParam);
        ECDH_LOG(INFO) << LOG_DESC("initTaskState") << printTaskInfo(task);
        return true;
    }
    catch (std::exception const& e)
    {
        ECDH_LOG(WARNING) << LOG_DESC("initTaskState exception")
                          << LOG_KV("msg", boost::diagnostic_information(e));
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandleTaskError,
            "init task error for " + boost::diagnostic_information(e));
        onTaskError("initTaskState", std::move(error), _taskState->peerID(), task->id(),
            dataResource ? dataResource->resourceID() : "emptyDataResource");
        return false;
    }
}

// the psi-server and psi-client load data from DataResource and blind the plainData into cipher
void EcdhPSIImpl::blindData(TaskState::Ptr const& _taskState)
{
    auto startT = utcSteadyTime();
    auto task = _taskState->task();
    bool server = (task->selfParty()->partyIndex() == (int)PartyType::Server) ? true : false;
    // the client cache is full
    if (!server && m_cache->isFull())
    {
        return;
    }
    auto dataResource = task->selfParty()->dataResource();
    uint32_t packetType = (int32_t)ECDHPacketType::EvaluateRequest;
    if (server)
    {
        packetType = (int32_t)ECDHPacketType::ServerBlindedData;
    }
    // load all data from the given resource
    uint64_t batchDataCount = 0;
    auto self = weak_from_this();
    auto const& taskID = task->id();
    auto serverCipherDataCache = m_cache->getServerCipherDataCache(taskID);
    do
    {
        // the task has been canceled
        if (!getTaskByID(taskID))
        {
            // erase the cache and return
            m_cache->eraseCache(taskID);
            return;
        }
        // not to blind when the cache is Full
        if (!server && m_cache->isFull())
        {
            ECDH_LOG(TRACE) << LOG_DESC("blindData return for the cache is full")
                            << LOG_KV("capacity", m_cache->capacity());
            break;
        }
        if (_taskState->loadFinished())
        {
            break;
        }
        int32_t seq = 0;
        DataBatch::Ptr dataBatch = nullptr;
        {
            bcos::Guard l(m_mutex);
            // Note: next is not thread-safe
            dataBatch = _taskState->reader()->next(_taskState->readerParam(), DataSchema::Bytes);
            if (!dataBatch)
            {
                _taskState->setFinished(true);
                break;
            }
            // allocate seq
            seq = _taskState->allocateSeq();
            if (_taskState->sqlReader())
            {
                _taskState->setFinished(true);
            }
        }
        auto ecdhTaskState = std::dynamic_pointer_cast<EcdhTaskState>(_taskState);
        auto batchPk = ecdhTaskState->ecdhCrypto()->batchGetPublicKey(dataBatch);

        auto evaluateRequest = m_config->msgFactory()->createPSIMessage(packetType);
        evaluateRequest->setData(std::move(batchPk));
        evaluateRequest->setResourceID(dataResource->resourceID());
        auto dataBatchSize = dataBatch->size();
        // the client should append the plainData into cache for get the plainData later
        if (!server)
        {
            m_cache->insertSubTaskCache(
                taskID, seq, _taskState, serverCipherDataCache, std::move(dataBatch));
        }
        ECDH_LOG(INFO) << LOG_DESC("blindData") << LOG_KV("resource", dataResource->resourceID())
                       << LOG_KV("seq", seq) << LOG_KV("size", dataBatchSize)
                       << LOG_KV("cacheCapacity", m_cache->capacity())
                       << LOG_KV("task", _taskState->task()->id());
        auto sendT = utcSteadyTime();
        m_config->generateAndSendPPCMessage(
            _taskState->peerID(), _taskState->task()->id(), evaluateRequest,
            [self, seq, _taskState, sendT](Error::Ptr&& _error) {
                if (!_error)
                {
                    ECDH_LOG(INFO) << LOG_DESC("blindData: send evaluate request success")
                                   << LOG_KV("seq", seq);
                    return;
                }
                auto psi = self.lock();
                if (!psi)
                {
                    return;
                }
                ECDH_LOG(WARNING) << LOG_DESC("blindData: send evaluate request error")
                                  << LOG_KV("seq", seq) << LOG_KV("code", _error->errorCode())
                                  << LOG_KV("msg", _error->errorMessage())
                                  << LOG_KV("timecost", (utcSteadyTime() - sendT));
                psi->cancelTask(std::move(_error), _taskState->task()->id());
            },
            seq);
        batchDataCount++;
    } while (m_started && !_taskState->sqlReader());
    if (!server)
    {
        return;
    }
    // the server send dataBatchCount to client
    ECDH_LOG(INFO) << LOG_DESC("blindData: sync the server-data-batch-count")
                   << LOG_KV("batchDataCount", batchDataCount);
    auto req =
        m_config->msgFactory()->createPSIMessage((uint32_t)ECDHPacketType::SyncDataBatchInfo);
    req->setDataBatchCount(batchDataCount);
    req->setResourceID(dataResource->resourceID());
    m_config->generateAndSendPPCMessage(_taskState->peerID(), _taskState->task()->id(), req,
        [self, _taskState](Error::Ptr&& _error) {
            if (!_error)
            {
                return;
            }
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            ECDH_LOG(WARNING) << LOG_DESC("blindData: send SyncDataBatchInfo request error")
                              << LOG_KV("code", _error->errorCode())
                              << LOG_KV("msg", _error->errorMessage());
            // cancel the task
            psi->cancelTask(std::move(_error), _taskState->task()->id());
        });
    ECDH_LOG(INFO) << LOG_DESC("blindData success") << printTaskInfo(_taskState->task())
                   << LOG_KV("timecost", (utcSteadyTime() - startT));
}

// receive the blinded-data from client, the psi-server response the evaluated data
void EcdhPSIImpl::handleEvaluateRequest(PSIMessageInterface::Ptr _msg)
{
    auto ret = checkAndObtainTaskState(_msg);
    if (!ret.first || !ret.second)
    {
        return;
    }
    auto taskState = ret.first;
    auto ecdhCrypto = ret.second;
    auto startT = utcSteadyTime();
    ECDH_LOG(INFO) << LOG_DESC("handleEvaluateRequest") << printPSIMessage(_msg);
    auto sharedPk = ecdhCrypto->batchGetSharedPublicKey(_msg->takeData());
    // response the evaluated result
    auto evaluateResponse =
        m_config->msgFactory()->createPSIMessage((uint32_t)ECDHPacketType::EvaluateResponse);
    evaluateResponse->setData(std::move(sharedPk));
    auto self = weak_from_this();
    auto startSendT = utcSteadyTime();
    m_config->generateAndSendPPCMessage(
        _msg->from(), _msg->taskID(), evaluateResponse,
        [self, _msg, startSendT](Error::Ptr&& _error) {
            if (!_error)
            {
                return;
            }
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            ECDH_LOG(WARNING) << LOG_DESC("handleEvaluateRequest: send evaluate response error")
                              << printPSIMessage(_msg) << LOG_KV("code", _error->errorCode())
                              << LOG_KV("msg", _error->errorMessage())
                              << LOG_KV("timecost", utcSteadyTime() - startSendT);
            psi->onTaskError("handleEvaluateRequest: response evaluate data", std::move(_error),
                _msg->from(), _msg->taskID(), _msg->resourceID());
        },
        _msg->seq());
    ECDH_LOG(INFO) << LOG_DESC("handleEvaluateRequest success") << printPSIMessage(_msg)
                   << LOG_KV("timecost", (utcSteadyTime() - startT));
}

std::pair<TaskState::Ptr, ECDHCrypto::Ptr> EcdhPSIImpl::checkAndObtainTaskState(
    PSIMessageInterface::Ptr _msg)
{
    auto taskState = getTaskByID(_msg->taskID());
    if (!taskState)
    {
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::TaskNotFound,
            "Task " + _msg->taskID() + " not found when handle server blinded-data!");
        onTaskError("handleServerBlindData", std::move(error), _msg->from(), _msg->taskID(),
            "emptyDataResource");
        return std::make_pair(nullptr, nullptr);
    }
    auto ecdhTaskState = std::dynamic_pointer_cast<EcdhTaskState>(taskState);
    // here the client maybe not ready for the ecdhCrypto when submit the multiple-tasks with same
    // taskID and some tasks failed
    if (!ecdhTaskState->ecdhCrypto())
    {
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::TaskNotReady,
            "The ecdh-crypto-suite is not ready for task " + _msg->taskID() +
                " not found when handle server blinded-data!");
        onTaskError("handleServerBlindData", std::move(error), _msg->from(), _msg->taskID(),
            "emptyDataResource");
        return std::make_pair(nullptr, nullptr);
    }
    return std::make_pair(taskState, ecdhTaskState->ecdhCrypto());
}

void EcdhPSIImpl::handleServerBlindData(PSIMessageInterface::Ptr _msg)
{
    auto startT = utcSteadyTime();
    ECDH_LOG(INFO) << LOG_DESC("handleServerBlindData") << printPSIMessage(_msg);
    auto ret = checkAndObtainTaskState(_msg);
    if (!ret.first || !ret.second)
    {
        return;
    }
    auto taskState = ret.first;
    auto ecdhCrypto = ret.second;
    auto sharedPk = ecdhCrypto->batchGetSharedPublicKey(_msg->takeData());

    // append the evaluated data into cache
    auto cache = m_cache->getServerCipherDataCache(_msg->taskID());
    if (!cache)
    {
        auto taskState = getTaskByID(_msg->taskID());
        if (!taskState)
        {
            auto error = BCOS_ERROR_PTR(PSIRetCode::TaskNotFound,
                m_config->selfParty() + " handleServerBlindData failed for task not found!");
            ECDH_LOG(WARNING) << LOG_DESC("handleServerBlindData failed")
                              << LOG_KV("error", error->errorCode())
                              << LOG_KV("msg", error->errorMessage()) << printPSIMessage(_msg);
            onTaskError("handleServerBlindData: Task not found", std::move(error), _msg->from(),
                _msg->taskID(), _msg->resourceID());
            return;
        }
        cache = m_cache->insertServerCipherCache(_msg->taskID(), taskState);
    }
    cache->appendServerCipherData(_msg->seq(), std::move(sharedPk));
    m_cache->tryToIntersectionAndStoreResult();
    ECDH_LOG(INFO) << LOG_DESC("handleServerBlindData success") << printPSIMessage(_msg)
                   << LOG_KV("timecost", (utcSteadyTime() - startT));
}

void EcdhPSIImpl::handleEvaluateResponse(PSIMessageInterface::Ptr _msg)
{
    auto startT = utcSteadyTime();
    auto evaluateData = _msg->takeData();
    ECDH_LOG(INFO) << LOG_DESC("handleEvaluateResponse") << printPSIMessage(_msg)
                   << LOG_KV("dataSize", evaluateData.size());

    // append the evaluate response into the cache
    auto cache = m_cache->getSubTaskCache(_msg->taskID(), _msg->seq());
    // if the cache is empty, this means the blindData for the (taskID, seq) has not been triggered
    // maybe this is evaluate response of the last-canceled task
    if (!cache)
    {
        return;
    }
    cache->setClientCipherData(std::move(evaluateData));
    m_cache->tryToIntersectionAndStoreResult();
    ECDH_LOG(INFO) << LOG_DESC("handleEvaluateResponse success") << printPSIMessage(_msg)
                   << LOG_KV("timecost", (utcSteadyTime() - startT));
}

void EcdhPSIImpl::onRecvSyncDataInfoMsg(PSIMessageInterface::Ptr const& _msg)
{
    ECDH_LOG(INFO) << LOG_DESC("onRecvSyncDataInfoMsg") << printPSIMessage(_msg)
                   << LOG_KV("dataBatchCount", _msg->dataBatchCount());
    auto cache = m_cache->getServerCipherDataCache(_msg->taskID());
    if (!cache)
    {
        auto error = BCOS_ERROR_PTR(
            PSIRetCode::TaskNotFound, "onRecvSyncDataInfoMsg failed for task not found!");
        onTaskError("onRecvSyncDataInfoMsg: Task not found", std::move(error), _msg->from(),
            _msg->taskID(), _msg->resourceID());
        return;
    }
    cache->setServerDataBatchCount(_msg->dataBatchCount());
    // try to intersection and store the psi result
    m_cache->tryToIntersectionAndStoreResult();
}

// the client receive the handshake response
void EcdhPSIImpl::onHandshakeResponse(PSIMessageInterface::Ptr const& _msg)
{
    auto handshakeResponse = std::dynamic_pointer_cast<PSIHandshakeResponse>(_msg);
    ECDH_LOG(INFO) << LOG_DESC("onHandshakeResponse") << LOG_KV("curve", handshakeResponse->curve())
                   << LOG_KV("hashImpl", handshakeResponse->hashType()) << printPSIMessage(_msg)
                   << LOG_KV("uuid", _msg->uuid());
    auto taskState = getTaskByID(_msg->taskID());
    if (!taskState)
    {
        ECDH_LOG(INFO) << LOG_DESC("onHandshakeResponse return for the task not found")
                       << printPSIMessage(_msg);
        auto error =
            BCOS_ERROR_PTR((int)PSIRetCode::TaskNotFound, "Task " + _msg->taskID() + " not found!");
        onTaskError("onHandshakeResponse", std::move(error), _msg->from(), _msg->taskID(),
            "emptyDataResource");
        return;
    }
    if (handshakeResponse->curve() == -1 || handshakeResponse->hashType() == -1)
    {
        auto errorMsg = "onHandshakeResponse return for not find the matching curve/hashType";
        ECDH_LOG(WARNING) << LOG_DESC(errorMsg) << printPSIMessage(handshakeResponse);
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandshakeFailed, errorMsg);
        auto dataResource = taskState->task()->selfParty()->dataResource();
        onTaskError("onHandshakeResponse", std::move(error), taskState->peerID(),
            taskState->task()->id(), dataResource ? dataResource->resourceID() : "empty");
        return;
    }
    // create and set the handshaked ecdh-crypto
    auto ecdhCrypto = m_config->ecdhCryptoFactory()->createECDHCrypto(
        handshakeResponse->curve(), handshakeResponse->hashType());
    auto ecdhTaskState = std::dynamic_pointer_cast<EcdhTaskState>(taskState);
    ecdhTaskState->setEcdhCrypto(ecdhCrypto);

    // response to the server, the server can blindData
    auto psiMsg =
        m_msgFactory->createTaskNotificationMessage((uint32_t)PSIPacketType::HandshakeSuccess);
    psiMsg->setErrorCode(0);
    psiMsg->setErrorMessage("success");
    auto startT = bcos::utcSteadyTime();
    m_config->asyncSendResponse(_msg->fromNode(), taskState->task()->id(), _msg->uuid(), psiMsg,
        [this, startT, _msg](bcos::Error::Ptr _error) {
            if (!_error || _error->errorCode() == 0)
            {
                PSI_FRAMEWORK_LOG(INFO) << LOG_DESC("asyncSendResponse success")
                                        << LOG_KV("timecost", (bcos::utcSteadyTime() - startT))
                                        << printPSIMessage(_msg) << LOG_KV("uuid", _msg->uuid());
                return;
            }
            PSI_FRAMEWORK_LOG(WARNING)
                << LOG_DESC("onHandshakeResponse: response-handshake result error")
                << LOG_KV("code", _error->errorCode()) << LOG_KV("msg", _error->errorMessage())
                << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
        });

    // blindData using specified ecc-curve and hash-algorithm
    triggerDataBlind(taskState);

    // Note: the sql will load all data at-one-time, no-need blindData repeatedly
    if (!taskState->sqlReader())
    {
        auto weakTaskState = std::weak_ptr<TaskState>(taskState);
        auto self = weak_from_this();
        taskState->setWorker([weakTaskState, self]() {
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
            psi->triggerDataBlind(state);
        });
    }
}

void EcdhPSIImpl::triggerDataBlind(TaskState::Ptr const& _taskState)
{
    auto self = weak_from_this();
    m_config->threadPool()->enqueue([self, _taskState]() {
        auto psi = self.lock();
        if (!psi)
        {
            return;
        }
        try
        {
            psi->blindData(_taskState);
        }
        catch (std::exception const& e)
        {
            ECDH_LOG(INFO) << LOG_DESC("runPSI exception")
                           << LOG_KV("error", boost::diagnostic_information(e))
                           << printTaskInfo(_taskState->task());
            auto dataResource = _taskState->task()->selfParty()->dataResource();
            auto error = BCOS_ERROR_PTR((int)PSIRetCode::BlindDataError,
                "ecdh-psi blindData error, msg: " + std::string(boost::diagnostic_information(e)));
            psi->onTaskError("runPSI-blindData", std::move(error), _taskState->peerID(),
                _taskState->task()->id(), dataResource ? dataResource->resourceID() : "empty");
        }
    });
}

// the server receive the handshake request and response
void EcdhPSIImpl::onHandshakeRequest(PSIMessageInterface::Ptr const& _msg)
{
    auto taskState = getTaskByID(_msg->taskID());
    if (!taskState)
    {
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::TaskNotFound,
            "task " + _msg->taskID() + " not found when receiving handshake request");
        onTaskError(
            "onHandshakeRequest", std::move(error), _msg->from(), _msg->taskID(), "emptyResource");
        return;
    }
    auto handshakeReq = std::dynamic_pointer_cast<PSIHandshakeRequest>(_msg);
    auto const& curves = handshakeReq->curves();
    auto const& hashList = handshakeReq->hashList();
    ECDH_LOG(INFO) << LOG_DESC("onHandshakeRequest") << LOG_KV("supportedCurves", curves.size())
                   << LOG_KV("supportedHashList", hashList.size()) << printPSIMessage(_msg);
    std::set<int> clientCurves(curves.begin(), curves.end());
    std::set<int> clientHashList(hashList.begin(), hashList.end());

    auto supportedCurves = g_PPCConfig.supportedCurves(
        (uint8_t)TaskType::PSI, (uint8_t)TaskAlgorithmType::ECDH_PSI_2PC);
    auto supportedHashList = g_PPCConfig.supportedHashList(
        (uint8_t)TaskType::PSI, (uint8_t)TaskAlgorithmType::ECDH_PSI_2PC);
    std::set<int> localCurves(supportedCurves.begin(), supportedCurves.end());
    std::set<int> localHashTypes(supportedHashList.begin(), supportedHashList.end());
    auto handshakeResponse =
        m_msgFactory->createHandshakeResponse((uint32_t)PSIPacketType::HandshakeResponse);
    bool curveSelected = false;
    bool hashTypeSelected = false;
    // sm-crypto enabled
    if (g_PPCConfig.smCrypto())
    {
        // try to select SM2
        if (clientCurves.count((int)ppc::protocol::ECCCurve::SM2) &&
            localCurves.count((int)ppc::protocol::ECCCurve::SM2))
        {
            handshakeResponse->setCurve((int)ppc::protocol::ECCCurve::SM2);
            curveSelected = true;
        }
        // try to select SM3
        if (clientHashList.count((int)ppc::protocol::HashImplName::SM3) &&
            localHashTypes.count((int)ppc::protocol::HashImplName::SM3))
        {
            handshakeResponse->setHashType((int)ppc::protocol::HashImplName::SM3);
            hashTypeSelected = true;
        }
    }
    if (!curveSelected)
    {
        handshakeResponse->setCurve(selectCryptoAlgorithm(handshakeReq->curves(), localCurves));
    }
    if (!hashTypeSelected)
    {
        handshakeResponse->setHashType(
            selectCryptoAlgorithm(handshakeReq->hashList(), localHashTypes));
    }
    auto ecdhTaskState = std::dynamic_pointer_cast<EcdhTaskState>(taskState);
    auto selectedCurve = handshakeResponse->curve();
    auto selectedHashType = handshakeResponse->hashType();

    auto dataResource = taskState->task()->selfParty()->dataResource();
    if (selectedCurve != -1 && selectedHashType != -1)
    {
        ECDH_LOG(INFO) << LOG_DESC("onHandshakeRequest success")
                       << LOG_KV("selectedCurve", (ppc::protocol::ECCCurve)selectedCurve)
                       << LOG_KV("selectedHashType", (ppc::protocol::HashImplName)selectedHashType);
        auto ecdhCrypto =
            m_config->ecdhCryptoFactory()->createECDHCrypto(selectedCurve, selectedHashType);
        ecdhTaskState->setEcdhCrypto(ecdhCrypto);
    }
    else
    {
        auto errorMsg = "onHandshakeRequest failed for no curve/hashType selected";
        ECDH_LOG(WARNING) << LOG_DESC(errorMsg) << LOG_KV("selectedCurve", selectedCurve)
                          << LOG_KV("selectedHashType", selectedHashType);
        auto error = BCOS_ERROR_PTR((int)PSIRetCode::HandshakeFailed, errorMsg);
        onTaskError("onHandshakeRequest", std::move(error), _msg->from(), _msg->taskID(),
            dataResource ? dataResource->resourceID() : "emptyResource");
        return;
    }
    auto self = weak_from_this();
    auto startT = bcos::utcSteadyTime();
    m_config->generateAndSendPPCMessage(
        _msg->from(), _msg->taskID(), handshakeResponse,
        [self, _msg, taskState, dataResource](Error::Ptr&& _error) {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            if (!_error || _error->errorCode() == 0)
            {
                return;
            }
            // response failed
            ECDH_LOG(ERROR) << LOG_DESC("onHandshakeRequest: send handshake response error")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("msg", _error->errorMessage()) << printPSIMessage(_msg);
            psi->onTaskError("onHandshakeRequest", std::move(_error), _msg->from(), _msg->taskID(),
                dataResource ? dataResource->resourceID() : "emptyResource");
        },
        0,
        [self, _msg, startT, taskState, dataResource](bcos::Error::Ptr _error,
            std::string const& _agencyID, ppc::front::PPCMessageFace::Ptr _ppcMsg,
            ppc::front::ResponseFunc) {
            auto psi = self.lock();
            if (!psi)
            {
                return;
            }
            if (!_error || _error->errorCode() == 0)
            {
                ECDH_LOG(INFO) << LOG_DESC(
                                      "onHandshakeRequest: send handshake response and receive the "
                                      "client-ack, begin triggerDataBlind")
                               << printTaskInfo(taskState->task())
                               << LOG_KV("uuid", _ppcMsg->uuid());
                // triggerDataBlind when receiving the client-ack
                psi->triggerDataBlind(taskState);
                return;
            }
            // response failed
            ECDH_LOG(ERROR) << LOG_DESC(
                                   "onHandshakeRequest: send handshake response and receive the "
                                   "client-ack error")
                            << LOG_KV("code", _error->errorCode())
                            << LOG_KV("timecost", (bcos::utcSteadyTime() - startT))
                            << LOG_KV("msg", _error->errorMessage()) << printPSIMessage(_msg);
            psi->onTaskError("onHandshakeRequest", std::move(_error), _msg->from(), _msg->taskID(),
                dataResource ? dataResource->resourceID() : "emptyResource");
        });
}

int EcdhPSIImpl::selectCryptoAlgorithm(
    std::vector<int> _clientSupportedAlgorithms, std::set<int> _localSupportedAlgorithms)
{
    for (auto const& algorithm : _clientSupportedAlgorithms)
    {
        if (_localSupportedAlgorithms.count(algorithm))
        {
            return algorithm;
        }
    }
    // Note: -1 means select algorithm failed
    return -1;
}