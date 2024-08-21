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
 * @file GatewayServiceServer.cpp
 * @author: shawnhe
 * @date 2022-10-21
 */
#include "GatewayServiceServer.h"
#include "FrontService.h"
#include "ppc-tars-protocol/client/FrontServiceClient.h"
#include "ppc-tars-protocol/ppc-tars-protocol/Common.h"

using namespace ppctars;
using namespace ppc::gateway;

ppctars::Error GatewayServiceServer::asyncSendMessage(
    const std::string& _agencyID, const vector<tars::Char>& _message, tars::TarsCurrentPtr _current)

{
    GATEWAYSERVER_LOG(TRACE) << LOG_DESC("receive message from front");

    _current->setResponse(false);

    auto ppcMessage = m_ppcMsgFactory->buildPPCMessage(
        bcos::bytesConstRef((bcos::byte const*)_message.data(), _message.size()));
    if (!ppcMessage)
    {
        auto error = ppctars::Error();
        error.errorCode = ppc::protocol::PPCRetCode::DECODE_PPC_MESSAGE_ERROR;
        error.errorMessage = "decode ppc message error";
        async_response_asyncSendMessage(_current, error);
        return error;
    }

    m_gateway->asyncSendMessage(_agencyID, ppcMessage, [_current](const bcos::Error::Ptr& error) {
        async_response_asyncSendMessage(_current, toTarsError(error));
    });

    return ppctars::Error();
}

ppctars::Error GatewayServiceServer::notifyTaskInfo(
    const ppctars::TaskInfo& _taskInfo, tars::TarsCurrentPtr _current)
{
    _current->setResponse(true);

    auto error = m_gateway->notifyTaskInfo(toGatewayTaskInfo(_taskInfo));

    return toTarsError(error);
}

ppctars::Error GatewayServiceServer::eraseTaskInfo(
    const std::string& _taskID, tars::TarsCurrentPtr _current)
{
    _current->setResponse(true);

    auto error = m_gateway->eraseTaskInfo(_taskID);

    return toTarsError(error);
}

ppctars::Error GatewayServiceServer::registerGateway(
    const std::vector<ppctars::GatewayInfo>& _gatewayList, tars::TarsCurrentPtr _current)
{
    _current->setResponse(true);

    std::vector<ppc::protocol::GatewayInfo> gatewayList;
    for (const auto& tarsGate : _gatewayList)
    {
        ppc::protocol::GatewayInfo gateway;
        gateway.agencyID = tarsGate.agencyID;
        gateway.endpoint = tarsGate.endpoint;
        gatewayList.push_back(gateway);
    }

    auto error = m_gateway->registerGateway(gatewayList);

    return toTarsError(error);
}

ppctars::Error GatewayServiceServer::asyncRegisterFront(
    std::string const& _endPoint, tars::TarsCurrentPtr _current)
{
    _current->setResponse(false);
    try
    {
        // obtain the frontServiceClient specified by _endPoint
        auto self = weak_from_this();
        auto prx = createServantProxy<FrontServicePrx>(tars::Application::getCommunicator().get(),
            _endPoint, [self, _endPoint](const tars::TC_Endpoint& ep) {
                auto gatewayServer = self.lock();
                if (!gatewayServer)
                {
                    return;
                }
                GATEWAYSERVER_LOG(INFO)
                    << LOG_DESC("UnRegisterFront for disconnect") << LOG_KV("endPoint", _endPoint);
                // unregister when disconnected
                gatewayServer->m_gateway->unregisterFront(_endPoint);
            });
        m_gateway->registerFront(_endPoint, std::make_shared<FrontServiceClient>(prx));
        // register success
        async_response_asyncRegisterFront(
            _current, ppctars::toTarsError<bcos::Error::Ptr>(nullptr));
    }
    catch (std::exception const& e)
    {
        GATEWAYSERVER_LOG(WARNING)
            << LOG_DESC("asyncRegisterFront exception") << LOG_KV("endPoint", _endPoint)
            << LOG_KV("exception", boost::diagnostic_information(e));
        auto bcosError = std::make_shared<bcos::Error>(-1,
            "asyncRegisterFront exception for " + std::string(boost::diagnostic_information(e)));
        async_response_asyncRegisterFront(_current, ppctars::toTarsError(bcosError));
    }
    return ppctars::Error();
}

ppctars::Error GatewayServiceServer::asyncGetAgencyList(
    std::vector<std::string>& _agencyList, tars::TarsCurrentPtr _current)
{
    _current->setResponse(false);
    m_gateway->asyncGetAgencyList(
        [_current](bcos::Error::Ptr _error, std::vector<std::string>&& _agencyList) {
            // response the agency-list
            async_response_asyncGetAgencyList(
                _current, ppctars::toTarsError<bcos::Error::Ptr>(_error), std::move(_agencyList));
        });
    return ppctars::Error();
}
