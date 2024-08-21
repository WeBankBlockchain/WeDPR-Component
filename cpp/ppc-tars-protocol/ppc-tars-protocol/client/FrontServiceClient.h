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
 * @brief client for front service
 * @file FrontServiceClient.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "FrontService.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-tars-protocol/ppc-tars-protocol/Common.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/RefDataContainer.h>

#include <utility>

#define FRONTCLIENT_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("FrontServiceClient")

namespace ppctars
{
class FrontServiceClient : public ppc::front::FrontInterface
{
public:
    void start() override {}
    void stop() override {}

    explicit FrontServiceClient(const ppctars::FrontServicePrx& proxy) : m_proxy(proxy) {}

    void onReceiveMessage(
        ppc::front::PPCMessageFace::Ptr _message, ppc::front::ErrorCallbackFunc _callback) override
    {
        class Callback : public FrontServicePrxCallback
        {
        public:
            explicit Callback(ppc::front::ErrorCallbackFunc callback)
              : m_callback(std::move(callback))
            {}

            void callback_onReceiveMessage(const ppctars::Error& ret) override
            {
                if (!m_callback)
                {
                    return;
                }
                m_callback(toBcosError(ret));
            }

            void callback_onReceiveMessage_exception(tars::Int32 ret) override
            {
                if (!m_callback)
                {
                    return;
                }
                m_callback(toBcosError(ret));
            }

        private:
            ppc::front::ErrorCallbackFunc m_callback;
        };

        auto startT = bcos::utcSteadyTime();
        // encode message to bytes
        bcos::bytes buffer;
        _message->encode(buffer);

        FRONTCLIENT_LOG(TRACE) << LOG_DESC("after decode")
                               << LOG_KV("taskType", unsigned(_message->taskType()))
                               << LOG_KV("algorithmType", unsigned(_message->algorithmType()))
                               << LOG_KV("messageType", unsigned(_message->messageType()))
                               << LOG_KV("seq", _message->seq())
                               << LOG_KV("taskID", _message->taskID());

        m_proxy->tars_set_timeout(c_networkTimeout)
            ->async_onReceiveMessage(
                new Callback(_callback), std::vector<char>(buffer.begin(), buffer.end()));
        BCOS_LOG(TRACE) << LOG_DESC("call front onReceiveMessage")
                        << LOG_KV("msgSize", buffer.size())
                        << LOG_KV("timecost", bcos::utcSteadyTime() - startT);
    }

    // Note: since ppc-front is integrated with the node, no-need to implement this method
    bcos::Error::Ptr notifyTaskInfo(ppc::protocol::GatewayTaskInfo::Ptr) override
    {
        throw std::runtime_error("notifyTaskInfo: unimplemented interface!");
    }
    
    // Note: since ppc-front is integrated with the node, no-need to implement this method
    // erase the task-info when task finished
    bcos::Error::Ptr eraseTaskInfo(std::string const&) override
    {
        throw std::runtime_error("eraseTaskInfo: unimplemented interface!");
    }

    // Note: since ppc-front is integrated with the node, no-need to implement this method
    void asyncSendMessage(const std::string&, ppc::front::PPCMessageFace::Ptr, uint32_t _timeout,
        ppc::front::ErrorCallbackFunc _callback, ppc::front::CallbackFunc _respCallback) override
    {
        throw std::runtime_error("asyncSendMessage: unimplemented interface!");
    }

    // Note: since ppc-front is integrated with the node, no-need to implement this method
    // send response when receiving message from given agencyID
    void asyncSendResponse(const std::string&, std::string const&, ppc::front::PPCMessageFace::Ptr,
        ppc::front::ErrorCallbackFunc) override
    {
        throw std::runtime_error("asyncSendResponse: unimplemented interface!");
    }

    // Note: since ppc-front is integrated with the node, no-need to implement this method
    void asyncGetAgencyList(ppc::front::GetAgencyListCallback) override
    {
        throw std::runtime_error("asyncGetAgencyList: unimplemented interface!");
    }

private:
    // 1800s
    const int c_networkTimeout = 1800000;

    ppctars::FrontServicePrx m_proxy;
};
}  // namespace ppctars
