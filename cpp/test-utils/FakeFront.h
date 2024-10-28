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
 * @brief fake ppc network message interface
 * @file FakeFront.h
 * @author: yujiechen
 * @date 2022-11-16
 */
#pragma once
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"

using namespace ppc::protocol;
using namespace ppc::front;
// using namespace ppc::psi;
using namespace ppc::task;

namespace ppc::test
{
class FakeFront : public FrontInterface
{
public:
    using Ptr = std::shared_ptr<FakeFront>;
    FakeFront() = default;
    ~FakeFront() override = default;

    void registerMessageHandler(uint8_t _taskType, uint8_t _algorithmType,
        std::function<void(front::PPCMessageFace::Ptr)> _handler) override
    {}

    void registerRA2018(std::string const& _agencyID, TaskFrameworkInterface::Ptr _psi)
    {
        std::cout << "### registerRA2018: " << _agencyID << std::endl;
        m_agencyToRA2018[_agencyID] = _psi;
    }

    void registerEcdhPSI(std::string const& _agencyID, TaskFrameworkInterface::Ptr _psi)
    {
        std::cout << "### registerEcdhPSI: " << _agencyID << std::endl;
        m_agencyToEcdhPSI[_agencyID] = _psi;
    }

    void registerEcdhMultiPSI(std::string const& _agencyID, TaskFrameworkInterface::Ptr _psi)
    {
        std::cout << "### registerEcdhMultiPSI: " << _agencyID << std::endl;
        m_agencyToEcdhMutliPSI[_agencyID] = _psi;
    }

    void registerLabeledPSI(std::string const& _agencyID, TaskFrameworkInterface::Ptr _psi)
    {
        m_agencyToLabeledPSI[_agencyID] = _psi;
    }

    void registerCM2020(std::string const& _agencyID, TaskFrameworkInterface::Ptr _psi)
    {
        m_agencyToCM2020[_agencyID] = _psi;
    }

    void registerOTPIR(std::string const& _agencyID, TaskFrameworkInterface::Ptr _pir)
    {
        m_agencyToOTPIR[_agencyID] = _pir;
    }

    void asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
        uint32_t _timeout, ErrorCallbackFunc _callback, CallbackFunc _responseCallback) override
    {
        m_uuid++;
        auto id = std::to_string(m_uuid);
        if (_responseCallback)
        {
            insertCallback(id, _responseCallback);
            _message->setUuid(id);
        }


        uint8_t type = _message->algorithmType();
        switch (type)
        {
        case uint8_t(TaskAlgorithmType::RA_PSI_2PC):
        {
            if (!m_agencyToRA2018.count(_agencyID))
            {
                _callback(std::make_shared<bcos::Error>(
                    -1, "RA_PSI_2PC asyncSendMessage error! The gateway '" + _agencyID +
                            "' is not reachable!"));
                return;
            }
            auto psi = m_agencyToRA2018.at(_agencyID);
            psi->onReceiveMessage(_message);
            _callback(nullptr);
            break;
        }
        case uint8_t(TaskAlgorithmType::LABELED_PSI_2PC):
        {
            if (!m_agencyToLabeledPSI.count(_agencyID))
            {
                _callback(std::make_shared<bcos::Error>(
                    -1, "LABELED_PSI_2PC asyncSendMessage error! The gateway " + _agencyID +
                            " is not reachable!"));
                return;
            }
            auto psi = m_agencyToLabeledPSI.at(_agencyID);
            psi->onReceiveMessage(_message);
            _callback(nullptr);
            break;
        }
        case uint8_t(TaskAlgorithmType::CM_PSI_2PC):
        {
            if (!m_agencyToCM2020.count(_agencyID))
            {
                _callback(std::make_shared<bcos::Error>(
                    -1, "CM_PSI_2PC asyncSendMessage error! The gateway " + _agencyID +
                            " is not reachable!"));
                return;
            }
            auto psi = m_agencyToCM2020.at(_agencyID);
            psi->onReceiveMessage(_message);
            _callback(nullptr);
            break;
        }
        case uint8_t(TaskAlgorithmType::ECDH_PSI_2PC):
        {
            if (!m_agencyToEcdhPSI.count(_agencyID))
            {
                _callback(std::make_shared<bcos::Error>(
                    -1, "ECDH_PSI_2PC asyncSendMessage error! The gateway " + _agencyID +
                            " is not reachable!"));
                return;
            }
            auto psi = m_agencyToEcdhPSI.at(_agencyID);
            psi->onReceiveMessage(_message);
            _callback(nullptr);
            break;
        }
        case uint8_t(TaskAlgorithmType::ECDH_PSI_MULTI):
        {
            if (!m_agencyToEcdhMutliPSI.count(_agencyID))
            {
                _callback(std::make_shared<bcos::Error>(
                    -1, "ECDH_PSI_MULTI asyncSendMessage error! The gateway " + _agencyID +
                            " is not reachable!"));
                return;
            }
            auto psi = m_agencyToEcdhMutliPSI.at(_agencyID);
            psi->onReceiveMessage(_message);
            _callback(nullptr);
            break;
        }
        case uint8_t(TaskAlgorithmType::OT_PIR_2PC):
        {
            if (!m_agencyToOTPIR.count(_agencyID))
            {
                _callback(std::make_shared<bcos::Error>(
                    -1, "OT_PIR_2PC asyncSendMessage error! The gateway " + _agencyID +
                            " is not reachable!"));
                return;
            }
            auto pir = m_agencyToOTPIR.at(_agencyID);
            pir->onReceiveMessage(_message);
            _callback(nullptr);
            break;
        }
        default:  // unreachable-branch
            break;
        }
    }

    bcos::Error::Ptr notifyTaskInfo(std::string const&) override { return nullptr; }

    // erase the task-info when task finished
    bcos::Error::Ptr eraseTaskInfo(std::string const&) override { return nullptr; }

    // send response when receiving message from given agencyID
    void asyncSendResponse(bcos::bytes const& peer, std::string const& _uuid,
        front::PPCMessageFace::Ptr _message, ErrorCallbackFunc _callback) override
    {
        if (m_uuidToCallback.count(_uuid))
        {
            auto callback = m_uuidToCallback[_uuid];
            removeCallback(_uuid);
            if (callback)
            {
                callback(nullptr, std::string(peer.begin(), peer.end()), _message, nullptr);
            }
        }
    }

    // for ut
    void setAgencyList(std::vector<std::string> const& agencyList)
    {
        bcos::WriteGuard l(x_agencyList);
        m_agencyList = agencyList;
    }

    std::vector<std::string> agencies() const override
    {
        bcos::ReadGuard l(x_agencyList);
        return m_agencyList;
    }

    void start() override {}
    void stop() override {}

private:
    // the uuid to _callback
    void insertCallback(std::string const& _id, CallbackFunc _callback)
    {
        bcos::Guard l(m_mutex);
        m_uuidToCallback[_id] = _callback;
    }

    void removeCallback(std::string const& _id)
    {
        bcos::Guard l(m_mutex);
        m_uuidToCallback.erase(_id);
    }

private:
    // Note: here assume that one agency has one psi
    std::map<std::string, TaskFrameworkInterface::Ptr> m_agencyToRA2018;

    std::map<std::string, TaskFrameworkInterface::Ptr> m_agencyToEcdhPSI;

    std::map<std::string, TaskFrameworkInterface::Ptr> m_agencyToEcdhMutliPSI;

    std::map<std::string, TaskFrameworkInterface::Ptr> m_agencyToLabeledPSI;

    std::map<std::string, TaskFrameworkInterface::Ptr> m_agencyToCM2020;

    std::map<std::string, TaskFrameworkInterface::Ptr> m_agencyToOTPIR;

    // uuid to callback
    std::map<std::string, CallbackFunc> m_uuidToCallback;
    bcos::Mutex m_mutex;
    std::atomic<int64_t> m_uuid = 0;

    // the agency list, for task-sync
    std::vector<std::string> m_agencyList;
    mutable bcos::SharedMutex x_agencyList;
};
}  // namespace ppc::test