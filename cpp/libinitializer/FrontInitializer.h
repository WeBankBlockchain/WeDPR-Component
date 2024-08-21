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
 * @file FrontInitializer.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#include "ppc-framework/front/Channel.h"
#include "ppc-framework/front/FrontInterface.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include "ppc-framework/rpc/RpcStatusInterface.h"
#include "ppc-front/ppc-front/Front.h"
#include "ppc-front/ppc-front/PPCChannelManager.h"
#include <bcos-utilities/IOServicePool.h>
#include <memory>

namespace ppc
{
namespace initializer
{
class FrontInitializer
{
public:
    using Ptr = std::shared_ptr<FrontInitializer>;

    explicit FrontInitializer(const std::string& _selfAgencyId,
        std::shared_ptr<bcos::ThreadPool> _threadPool,
        front::PPCMessageFaceFactory::Ptr _messageFactory)
      : m_selfAgencyId(_selfAgencyId),
        m_threadPool(std::move(_threadPool)),
        m_messageFactory(std::move(_messageFactory)),
        m_gatewayReporter(std::make_shared<bcos::Timer>(3000, "gatewayReporter"))
    {
        init();
    }

    virtual ~FrontInitializer() { stop(); }

    virtual void start();
    virtual void stop();

    const std::string& selfAgencyId() const { return m_selfAgencyId; }
    std::shared_ptr<bcos::ThreadPool> threadPool() { return m_threadPool; }

    front::Front::Ptr front() { return m_front; }

    front::FrontFactory::Ptr frontFactory() { return m_frontFactory; }

    front::PPCMessageFaceFactory::Ptr messageFactory() { return m_messageFactory; }

    void setRpcStatus(rpc::RpcStatusInterface::Ptr _status) { m_rpcStatus = std::move(_status); }

protected:
    void init();
    // report the gateway endpoints of other parties to the gateway periodically
    // Note: must set gatewayInterface into the front before call this function
    // Note: must set RpcStorageInterface into the FrontInitializer before call this function
    virtual void reportOtherGateway()
    {
        try
        {
            auto gatewayList = m_rpcStatus->listGateway();
            if (!gatewayList.empty())
            {
                m_front->gatewayInterface()->registerGateway(gatewayList);
            }
        }
        catch (std::exception const& e)
        {
            FRONT_LOG(WARNING) << LOG_DESC("reportOtherGateway exception")
                               << LOG_KV("exception", boost::diagnostic_information(e));
        }
        m_gatewayReporter->restart();
    }

protected:
    std::string m_selfAgencyId;
    std::shared_ptr<bcos::ThreadPool> m_threadPool;
    front::PPCMessageFaceFactory::Ptr m_messageFactory;

    front::Front::Ptr m_front;
    front::FrontFactory::Ptr m_frontFactory;

    std::atomic_bool m_running = {false};
    std::shared_ptr<bcos::Timer> m_gatewayReporter;
    rpc::RpcStatusInterface::Ptr m_rpcStatus;
};
}  // namespace initializer
}  // namespace ppc