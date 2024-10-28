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
 * @file PSIConfig.h
 * @author: yujiechen
 * @date 2022-10-25
 */
#pragma once
#include "Common.h"
#include "bcos-utilities/Common.h"
#include "ppc-framework/Helper.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/io/DataResourceLoader.h"
#include "ppc-framework/protocol/Protocol.h"
#include "psi-framework/interfaces/PSIMessageInterface.h"
#include <future>
#include <utility>

namespace ppc::psi
{
class PSIConfig
{
public:
    using Ptr = std::shared_ptr<PSIConfig>;
    PSIConfig(ppc::protocol::TaskAlgorithmType _algorithmType,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader)
      : m_algorithmType(_algorithmType), m_dataResourceLoader(std::move(_dataResourceLoader))
    {}

    PSIConfig(ppc::protocol::TaskAlgorithmType _algorithmType, const std::string& _selfParty,
        ppc::front::FrontInterface::Ptr _front,
        ppc::front::PPCMessageFaceFactory::Ptr _ppcMsgFactory,
        ppc::io::DataResourceLoader::Ptr _dataResourceLoader, int _holdingMessageMinutes)
      : m_algorithmType(_algorithmType),
        m_selfParty(_selfParty),
        m_front(std::move(_front)),
        m_ppcMsgFactory(std::move(_ppcMsgFactory)),
        m_dataResourceLoader(std::move(_dataResourceLoader)),
        m_networkTimeout(_holdingMessageMinutes * 60 * 1000),
        m_taskExpireTime(m_networkTimeout)
    {
        PSI_LOG(INFO) << LOG_DESC("create PSIConfig") << LOG_KV("algorithmType", m_algorithmType)
                      << LOG_KV("holdingMessageMinutes", _holdingMessageMinutes)
                      << LOG_KV("networkTimeout", m_networkTimeout)
                      << LOG_KV("taskExpireTime", m_taskExpireTime);
    }

    virtual ~PSIConfig() = default;

    ppc::front::FrontInterface::Ptr const& front() const { return m_front; }
    std::string selfParty() const { return m_selfParty; }

    ppc::protocol::TaskAlgorithmType algorithmType() const { return m_algorithmType; }
    ppc::front::PPCMessageFaceFactory::Ptr const& ppcMsgFactory() const { return m_ppcMsgFactory; }
    int networkTimeout() const { return m_networkTimeout; }

    bcos::Error::Ptr sendMessage(std::string const& _receiver, front::PPCMessageFace::Ptr _message)
    {
        std::promise<bcos::Error::Ptr> completedFuture;
        m_front->asyncSendMessage(
            _receiver, std::move(_message), m_networkTimeout,
            [&completedFuture](
                const bcos::Error::Ptr& _error) { completedFuture.set_value(_error); },
            nullptr);

        return completedFuture.get_future().get();
    }

    void generateAndSendPPCMessage(std::string const& _peerID, std::string const& _taskID,
        PSIMessageInterface::Ptr const& _msg, std::function<void(bcos::Error::Ptr&&)> _callback,
        uint32_t _seq = 0, ppc::front::CallbackFunc _responseCallback = nullptr)
    {
        auto ppcMsg = generatePPCMsg(_taskID, _msg, _seq);
        PSI_LOG(TRACE) << LOG_DESC("generateAndSendPPCMessage") << LOG_KV("peer", _peerID)
                       << printPSIMessage(_msg) << LOG_KV("msgType", (int)_msg->packetType())
                       << LOG_KV("seq", _seq);
        m_front->asyncSendMessage(
            _peerID, ppcMsg, m_networkTimeout,
            [_callback](bcos::Error::Ptr _error) {
                if (_callback)
                {
                    _callback(std::move(_error));
                }
            },
            _responseCallback);
    }

    void asyncSendResponse(bcos::bytes const& fromNode, std::string const& _taskID,
        std::string const& _uuid, PSIMessageInterface::Ptr const& _msg,
        ppc::front::ErrorCallbackFunc _callback, uint32_t _seq = 0)
    {
        auto ppcMsg = generatePPCMsg(_taskID, _msg, _seq);
        PSI_LOG(TRACE) << LOG_DESC("sendResponse") << LOG_KV("peer", printNodeID(fromNode))
                       << printPPCMsg(ppcMsg) << LOG_KV("msgType", (int)_msg->packetType())
                       << LOG_KV("uuid", _uuid);
        m_front->asyncSendResponse(fromNode, _uuid, ppcMsg, _callback);
    }

    ppc::io::DataResourceLoader::Ptr const& dataResourceLoader() const
    {
        return m_dataResourceLoader;
    }

    int taskExpireTime() const { return m_taskExpireTime; }
    void setTaskExpireTime(int _taskExpireTime) { m_taskExpireTime = _taskExpireTime; }

    std::vector<std::string> agencyList() const { return m_front->agencies(); }

protected:
    ppc::front::PPCMessageFace::Ptr generatePPCMsg(
        std::string const& _taskID, PSIMessageInterface::Ptr const& _msg, uint32_t _seq)
    {
        if (_msg->partyID().empty())
        {
            _msg->setPartyID(m_selfParty);
        }
        auto encodedData = _msg->encode();
        auto ppcMsg = m_ppcMsgFactory->buildPPCMessage();
        ppcMsg->setTaskType((uint8_t)ppc::protocol::TaskType::PSI);
        ppcMsg->setAlgorithmType((uint8_t)m_algorithmType);
        ppcMsg->setTaskID(_taskID);
        ppcMsg->setSender(m_selfParty);
        ppcMsg->setData(encodedData);
        ppcMsg->setSeq(_seq);
        return ppcMsg;
    }

protected:
    // the psi-alogrithm-type
    ppc::protocol::TaskAlgorithmType m_algorithmType;
    std::string m_selfParty;
    ppc::front::FrontInterface::Ptr m_front;
    // the front message factory
    ppc::front::PPCMessageFaceFactory::Ptr m_ppcMsgFactory;
    ppc::io::DataResourceLoader::Ptr m_dataResourceLoader;

    // the network-timeout, default 300s
    int m_networkTimeout = 300000;

    // the task-expire time
    int m_taskExpireTime = 10000;
};
}  // namespace ppc::psi