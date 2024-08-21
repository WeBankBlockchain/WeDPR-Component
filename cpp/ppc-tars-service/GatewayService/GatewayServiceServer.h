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
 * @file GatewayServiceServer.h
 * @author: shawnhe
 * @date 2022-10-21
 */
#pragma once

#include "GatewayInitializer.h"
#include "GatewayService.h"
#include "ppc-framework/Common.h"
#include "ppc-framework/gateway/GatewayInterface.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include <bcos-utilities/Log.h>

#define GATEWAYSERVER_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("GatewayServiceServer")

namespace ppctars
{
struct GatewayServiceParam
{
    ppc::gateway::GatewayInterface::Ptr gateway;
    ppc::front::PPCMessageFaceFactory::Ptr ppcMsgFactory;
};

class GatewayServiceServer : public ppctars::GatewayService,
                             public std::enable_shared_from_this<GatewayServiceServer>
{
public:
    using Ptr = std::shared_ptr<GatewayServiceServer>;

    explicit GatewayServiceServer(GatewayServiceParam const& _param)
      : m_gateway(_param.gateway), m_ppcMsgFactory(_param.ppcMsgFactory)
    {}
    void initialize() override {}
    void destroy() override {}

public:
    ppctars::Error asyncSendMessage(const std::string& _agencyID,
        const vector<tars::Char>& _message, tars::TarsCurrentPtr _current) override;

    ppctars::Error notifyTaskInfo(
        const ppctars::TaskInfo& _taskInfo, tars::TarsCurrentPtr _current) override;

    ppctars::Error eraseTaskInfo(
        const std::string& _taskID, tars::TarsCurrentPtr _current) override;

    ppctars::Error registerGateway(const std::vector<ppctars::GatewayInfo>& _gatewayList,
        tars::TarsCurrentPtr _current) override;

    ppctars::Error asyncRegisterFront(
        std::string const& _endPoint, tars::TarsCurrentPtr _current) override;

    ppctars::Error asyncGetAgencyList(
        std::vector<std::string>& _agencyList, tars::TarsCurrentPtr _current) override;

private:
    ppc::gateway::GatewayInterface::Ptr m_gateway;
    ppc::front::PPCMessageFaceFactory::Ptr m_ppcMsgFactory;
};
}  // namespace ppctars
