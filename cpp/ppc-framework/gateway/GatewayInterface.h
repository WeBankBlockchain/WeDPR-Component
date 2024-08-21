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
 * @brief interface for Gateway module
 * @file GatewayInterface.h
 * @author: shawnhe
 * @date 2022-10-19
 */
#pragma once
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include "ppc-framework/protocol/Task.h"
#include <bcos-utilities/Error.h>

namespace ppc
{
namespace gateway
{
using ErrorCallbackFunc = std::function<void(bcos::Error::Ptr)>;

/**
 * @brief: A list of interfaces provided by the gateway which are called by the front service.
 */
class GatewayInterface
{
public:
    using Ptr = std::shared_ptr<GatewayInterface>;
    GatewayInterface() = default;
    virtual ~GatewayInterface() {}

    /**
     * @brief: start/stop service
     */
    virtual void start() = 0;
    virtual void stop() = 0;

public:
    /**
     * @brief: send message to gateway
     * @param _agencyID: agency ID of receiver
     * @param _message: ppc message data
     * @param _callback: callback
     * @return void
     */
    virtual void asyncSendMessage(const std::string& _agencyID, front::PPCMessageFace::Ptr _message,
        ErrorCallbackFunc _callback) = 0;


    /**
     * @brief notice task info to gateway
     * @param _taskInfo the latest task information
     */
    virtual bcos::Error::Ptr notifyTaskInfo(protocol::GatewayTaskInfo::Ptr _taskInfo) = 0;

    // erase the task-info when task finished
    virtual bcos::Error::Ptr eraseTaskInfo(std::string const& _taskID) = 0;

    // register the gateway info of other parties
    virtual bcos::Error::Ptr registerGateway(
        const std::vector<ppc::protocol::GatewayInfo>& _gatewayList) = 0;

    virtual void registerFront(std::string const& _endPoint, front::FrontInterface::Ptr _front) = 0;
    virtual void unregisterFront(std::string const&) {}

    virtual void asyncGetAgencyList(ppc::front::GetAgencyListCallback _callback) = 0;
};

}  // namespace gateway
}  // namespace ppc
