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
 * @brief client for gateway service
 * @file GatewayServiceClient.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "GatewayService.h"
#include "ppc-framework/Common.h"
#include "ppc-framework/gateway/GatewayInterface.h"
#include "ppc-framework/protocol/Protocol.h"
#include "ppc-tars-protocol/Common.h"
#include "ppc-tars-protocol/TarsServantProxyCallback.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>
#include <bcos-utilities/RefDataContainer.h>

#include <utility>

#define GATEWAYCLIENT_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("GatewayServiceClient")

namespace ppctars
{
class GatewayServiceClient : public ppc::gateway::GatewayInterface
{
public:
    void start() override {}
    void stop() override {}

    explicit GatewayServiceClient(std::string const& _gatewayServiceName,
        const ppctars::GatewayServicePrx& _prx, int _holdingMessageMinutes)
      : m_gatewayServiceName(_gatewayServiceName),
        m_prx(_prx),
        m_networkTimeout(_holdingMessageMinutes * 60 * 1000)  // convert to ms
    {
        BCOS_LOG(INFO) << LOG_DESC("GatewayServiceClient")
                       << LOG_KV("networkTimeout", m_networkTimeout);
    }


    void asyncSendMessage(const std::string& _agencyID, ppc::front::PPCMessageFace::Ptr _message,
        ppc::gateway::ErrorCallbackFunc _callback) override
    {
        class Callback : public GatewayServicePrxCallback
        {
        public:
            explicit Callback(ppc::gateway::ErrorCallbackFunc callback)
              : m_callback(std::move(callback))
            {}

            void callback_asyncSendMessage(const ppctars::Error& ret) override
            {
                s_tarsTimeoutCount.store(0);
                if (!m_callback)
                {
                    return;
                }
                m_callback(toBcosError(ret));
            }

            void callback_asyncSendMessage_exception(tars::Int32 ret) override
            {
                s_tarsTimeoutCount++;
                if (!m_callback)
                {
                    return;
                }
                m_callback(toBcosError(ret));
            }

        private:
            ppc::gateway::ErrorCallbackFunc m_callback;
        };

        // encode message to bytes
        bcos::bytes buffer;
        _message->encode(buffer);

        GATEWAYCLIENT_LOG(TRACE) << LOG_DESC("send message to gateway by client")
                                 << LOG_KV("taskType", unsigned(_message->taskType()))
                                 << LOG_KV("algorithmType", unsigned(_message->algorithmType()))
                                 << LOG_KV("messageType", unsigned(_message->messageType()))
                                 << LOG_KV("seq", _message->seq())
                                 << LOG_KV("taskID", _message->taskID())
                                 << LOG_KV("receiver", _agencyID);

        m_prx->tars_set_timeout(m_networkTimeout)
            ->async_asyncSendMessage(new Callback(_callback), _agencyID,
                std::vector<char>(buffer.begin(), buffer.end()));
    }

    bcos::Error::Ptr notifyTaskInfo(ppc::protocol::GatewayTaskInfo::Ptr _taskInfo) override
    {
        auto tarsTaskInfo = toTarsTaskInfo(_taskInfo);
        auto activeEndPoints = tarsProxyActiveEndPoints(m_prx);
        // broadcast to all gateways
        uint errorCount = 0;
        std::string lastErrorMsg;
        for (auto& endPoint : activeEndPoints)
        {
            auto prx =
                ppctars::createServantProxy<GatewayServicePrx>(m_gatewayServiceName, endPoint);
            auto error = prx->tars_set_timeout(m_networkTimeout)->notifyTaskInfo(tarsTaskInfo);
            if (error.errorCode)
            {
                ++errorCount;
                lastErrorMsg = error.errorMessage;
            }
        }
        if (errorCount)
        {
            return std::make_shared<bcos::Error>(
                ppc::protocol::PPCRetCode::NOTIFY_TASK_ERROR, lastErrorMsg);
        }

        return nullptr;
    }

    bcos::Error::Ptr eraseTaskInfo(std::string const& _taskID) override
    {
        auto activeEndPoints = tarsProxyActiveEndPoints(m_prx);
        // broadcast to all gateways
        uint errorCount = 0;
        for (auto& endPoint : activeEndPoints)
        {
            try
            {
                auto prx =
                    ppctars::createServantProxy<GatewayServicePrx>(m_gatewayServiceName, endPoint);
                auto error = prx->tars_set_timeout(m_networkTimeout)->eraseTaskInfo(_taskID);
                if (error.errorCode)
                {
                    ++errorCount;
                }
            }
            catch (std::exception const& e)
            {
                ++errorCount;
                BCOS_LOG(INFO) << LOG_DESC("eraseTaskInfo exception")
                               << LOG_KV("exception", boost::diagnostic_information(e));
            }
        }
        if (errorCount)
        {
            return std::make_shared<bcos::Error>(
                -1, "eraseTaskInfo: error count:  " + std::to_string(errorCount));
        }

        return nullptr;
    }

    bcos::Error::Ptr registerGateway(
        const std::vector<ppc::protocol::GatewayInfo>& _gatewayList) override
    {
        std::vector<ppctars::GatewayInfo> gatewayList;
        for (const auto& gateway : _gatewayList)
        {
            ppctars::GatewayInfo tarsGate;
            tarsGate.agencyID = gateway.agencyID;
            tarsGate.endpoint = gateway.endpoint;
            gatewayList.push_back(tarsGate);
        }
        auto activeEndPoints = tarsProxyActiveEndPoints(m_prx);
        // broadcast to all gateways
        uint errorCount = 0;
        std::string lastErrorMsg;
        for (auto& endPoint : activeEndPoints)
        {
            auto prx =
                ppctars::createServantProxy<GatewayServicePrx>(m_gatewayServiceName, endPoint);
            auto error = prx->tars_set_timeout(m_networkTimeout)->registerGateway(gatewayList);
            if (error.errorCode)
            {
                ++errorCount;
                lastErrorMsg = error.errorMessage;
            }
        }
        if (errorCount)
        {
            return std::make_shared<bcos::Error>(
                ppc::protocol::PPCRetCode::REGISTER_GATEWAY_URL_ERROR, lastErrorMsg);
        }

        return nullptr;
    }


    void registerFront(std::string const& _endPoint, ppc::front::FrontInterface::Ptr) override
    {
        // Error registerFront(string endPoint);
        class Callback : public GatewayServicePrxCallback
        {
        public:
            explicit Callback(std::function<void(bcos::Error::Ptr)> _callback)
              : GatewayServicePrxCallback(), m_callback(_callback)
            {}
            ~Callback() override {}

            void callback_asyncRegisterFront(const ppctars::Error& ret) override
            {
                m_callback(toBcosError(ret));
            }
            void callback_asyncRegisterFront_exception(tars::Int32 ret) override
            {
                m_callback(toBcosError(ret));
            }

        private:
            std::function<void(bcos::Error::Ptr)> m_callback;
        };
        auto startT = bcos::utcSteadyTime();
        auto callback = [_endPoint, startT](bcos::Error::Ptr _error) {
            if (!_error || _error->errorCode() == 0)
            {
                GATEWAYCLIENT_LOG(TRACE)
                    << LOG_DESC("registerFront success") << LOG_KV("endPoint", _endPoint)
                    << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
                return;
            }
            GATEWAYCLIENT_LOG(INFO)
                << LOG_DESC("registerFront failed") << LOG_KV("code", _error->errorCode())
                << LOG_KV("endPoint", _endPoint)
                << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
        };
        m_prx->tars_set_timeout(m_networkTimeout)
            ->async_asyncRegisterFront(new Callback(callback), _endPoint);
    }

    void asyncGetAgencyList(ppc::front::GetAgencyListCallback _callback) override
    {
        class Callback : public GatewayServicePrxCallback
        {
        public:
            explicit Callback(ppc::front::GetAgencyListCallback _callback)
              : GatewayServicePrxCallback(), m_callback(_callback)
            {}
            ~Callback() override {}

            void callback_asyncGetAgencyList(
                const ppctars::Error& ret, std::vector<std::string> const& _agencyList) override
            {
                auto tmpAgencyList = _agencyList;
                m_callback(toBcosError(ret), std::move(tmpAgencyList));
            }
            void callback_asyncGetAgencyList_exception(tars::Int32 ret) override
            {
                std::vector<std::string> emptyAgencyList;
                m_callback(toBcosError(ret), std::move(emptyAgencyList));
            }

        private:
            ppc::front::GetAgencyListCallback m_callback;
        };
        m_prx->async_asyncGetAgencyList(new Callback(_callback));
    }

    // Note: unregisterFront is the function of the front-inner, no need to implement the client
    void unregisterFront(std::string const&) override
    {
        throw std::runtime_error("unregisterFront: unimplemented interface!");
    }


protected:
    static bool shouldStopCall() { return (s_tarsTimeoutCount >= c_maxTarsTimeoutCount); }


private:
    std::string m_gatewayServiceName;
    ppctars::GatewayServicePrx m_prx;

    // 1800s
    int m_networkTimeout = 1800000;
    std::string const c_moduleName = "GatewayServiceClient";

    static std::atomic<int64_t> s_tarsTimeoutCount;
    static const int64_t c_maxTarsTimeoutCount;
};
}  // namespace ppctars