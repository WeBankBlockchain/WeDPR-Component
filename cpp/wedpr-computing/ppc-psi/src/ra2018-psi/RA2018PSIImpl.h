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
 * @file RA2018PSIImpl.h
 * @author: yujiechen
 * @date 2022-10-25
 */
//
//               server                         client
//                         Offline
//  FullEvaluate
//  CF.insert(evaluated items)
//                          send CF
//                 ----------------------->
// ======================================================
//                         Online
//                                                 Blind
//                 blinded items(batch_size)
//                 <----------------------
//  Evaluate
//                   evaluated items
//                 ----------------------->
//                                              Finalize
// =======================================================
//                                           Intersection
#pragma once
#include "../psi-framework/PSIFramework.h"
#include "../psi-framework/TaskState.h"
#include "RA2018PSIConfig.h"
#include "core/CuckooFiltersCache.h"
#include "core/RA2018PSIClient.h"
#include "core/RA2018PSIServer.h"
#include "ppc-framework/protocol/Task.h"
#include "storage/RA2018PSIStorage.h"
#include <bcos-utilities/CallbackCollectionHandler.h>
#include <memory>

namespace ppc::psi
{
class RA2018PSIImpl : public PSIFramework, public std::enable_shared_from_this<RA2018PSIImpl>
{
public:
    using Ptr = std::shared_ptr<RA2018PSIImpl>;
    RA2018PSIImpl(RA2018PSIConfig::Ptr const& _config, RA2018PSIStorage::Ptr const& _storage,
        unsigned _idleTimeMs = 0, bool _waitResult = false, bool _disabled = false);
    ~RA2018PSIImpl() override = default;

    // run task
    void asyncRunTask(ppc::protocol::Task::ConstPtr _task,
        ppc::task::TaskResponseCallback&& _onTaskFinished) override;

    void start() override;
    void stop() override;

protected:
    // run data-processing task to generate and store cuckoo-filter
    void asyncRunDataProcessing(std::string const& _taskID,
        ppc::protocol::DataResource::ConstPtr const& _dataResource, int _operation,
        ppc::task::TaskResponseCallback const& _callback);
    // run psi task to generate data-intersection
    virtual void runPSI(TaskState::Ptr const& _taskState);
    virtual void runClientPSI(TaskState::Ptr const& _taskState);

    virtual void offlineFullEvaluate(std::string const& _resourceID, int _operation,
        ppc::io::LineReader::Ptr _reader, std::function<void(bcos::Error::Ptr&&)> _callback);

    virtual void evaluateAndInsert(std::string const& _resourceID, ppc::io::LineReader::Ptr _reader,
        CuckooFilterInfoSet&& _reusableFilters, std::function<void(bcos::Error::Ptr&&)> _callback);
    virtual void evaluateAndDelete(std::string const& _resourceID, ppc::io::LineReader::Ptr _reader,
        CuckooFilterInfoSet&& _filterInfos, std::function<void(bcos::Error::Ptr&&)> _callback);

    /////// the function to handle all ra2018 mesage
    void handlePSIMsg(PSIMessageInterface::Ptr _msg) override;

    /////// online PSI: client
    // sync the cuckoo-filter from the server
    virtual void syncCuckooFilter(std::string const& _peerID, std::string const& _taskID,
        ppc::protocol::DataResource::ConstPtr _dataResource);
    // handle the cuckoo-filter response
    virtual void handleCuckooFilterResponse(PSIMessageInterface::Ptr _msg);
    virtual bool handleCuckooFilterResponseItem(RA2018FilterMessage::Ptr _msg,
        CuckooFilterState::Ptr _cuckooFilterState, CuckooFilterInfo::Ptr const& _cuckooFilterInfo);
    // load data from DataResource and blind the plainData into cipher
    virtual bool blindData(
        std::string const& _cuckooFilterResourceID, TaskState::Ptr const& _taskState);
    // handle the evaluate data responsed from the server, try to finalize and intersection
    virtual void handleEvaluateResponse(PSIMessageInterface::Ptr _msg);


    /////// online PSI: Server
    // receive cuckoo-filter request, response the delta-cuckoo-filter
    virtual void handleCuckooFilterRequest(PSIMessageInterface::Ptr _msg);
    virtual void handleMissedCuckooFilterRequest(PSIMessageInterface::Ptr _msg);
    // receive the evaluate request, evaluate and response
    virtual void handleEvaluateRequest(PSIMessageInterface::Ptr _msg);

    // TODO: check this logic
    bool needLockResource(int _command, int _partyType) override
    {
        // Note: the psi server no need to lock the resource and record the pendingTasks
        if (_command == RA2018Command::RUN_PSI &&
            _partyType == (int)ppc::protocol::PartyType::Server)
        {
            return false;
        }
        return true;
    }

    void responseCuckooFilter(uint32_t _responseType, PSIMessageInterface::Ptr const& _msg,
        CuckooFilterInfo::Ptr const& _filterInfo, uint64_t _filterInfoSize, size_t _seq);

    template <typename T>
    void requestCuckooFilters(std::string const& _peerID, std::string const& _taskID,
        std::string const& _resourceID, T const& _filterInfos, uint32_t _type,
        int64_t _cuckooFilterSize = 0, uint32_t _seq = 0)
    {
        auto ra2018Msg = m_config->ra2018MsgFactory()->createRA2018FilterMessage(_type);
        std::vector<CuckooFilterInfo::Ptr> filterInfoList(_filterInfos.begin(), _filterInfos.end());
        ra2018Msg->setPartyID(m_config->selfParty());
        ra2018Msg->setFilterInfo(std::move(filterInfoList));
        ra2018Msg->setResourceID(_resourceID);
        ra2018Msg->setCuckooFilterSize(_cuckooFilterSize);
        auto self = weak_from_this();
        m_config->generateAndSendPPCMessage(
            _peerID, _taskID, ra2018Msg,
            [self, _peerID, _taskID, _resourceID](bcos::Error::Ptr&& _error) {
                if (!_error || _error->errorCode() == 0)
                {
                    return;
                }
                auto psi = self.lock();
                if (!psi)
                {
                    return;
                }
                psi->onTaskError("requestCuckooFilters: send cuckoo-filter request",
                    std::move(_error), _peerID, _taskID, _resourceID);
            },
            _seq);
    }

    // receive the handshake response
    void onHandshakeResponse(PSIMessageInterface::Ptr const& _msg) override
    {
        throw std::runtime_error("RA2018PSIImpl: unimplemented onHandshakeResponse!");
    }
    // receive the handshake request
    void onHandshakeRequest(PSIMessageInterface::Ptr const& _msg) override
    {
        throw std::runtime_error("RA2018PSIImpl: unimplemented onHandshakeRequest!");
    }

protected:
    bool m_disabled = false;
    RA2018PSIConfig::Ptr m_config;
    RA2018PSIStorage::Ptr m_storage;
    RA2018PSIServer::Ptr m_server;
    RA2018PSIClient::Ptr m_client;
    // the cuckoo-filter cache
    // cache for psi-client
    CuckooFiltersCache::Ptr m_cuckooFiltersCache;

    // the handler triggered when the RA2018PSIClient capacity decreased or some tasks execute
    // success
    bcos::Handler<> m_notifier;

    bcos::ThreadPool::Ptr m_worker;
    // the flag means that response to the sdk once handling the task or after the task completed
    bool m_waitResult;
};
}  // namespace ppc::psi
