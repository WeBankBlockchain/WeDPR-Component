/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file FrontServer.cpp
 * @author: yujiechen
 * @date 2024-09-03
 */
#include "FrontServer.h"
#include "Common.h"
#include "protobuf/src/RequestConverter.h"
#include <bcos-utilities/Common.h>

using namespace ppc::proto;
using namespace ppc::protocol;
using namespace grpc;

ServerUnaryReactor* FrontServer::onReceiveMessage(
    CallbackServerContext* context, const ReceivedMessage* receivedMsg, ppc::proto::Error* reply)
{
    ServerUnaryReactor* reactor(context->DefaultReactor());
    try
    {
        // decode the request
        auto msg = m_msgBuilder->build(bcos::bytesConstRef(
            (bcos::byte*)receivedMsg->data().data(), receivedMsg->data().size()));
        m_front->onReceiveMessage(msg, [reactor, reply](bcos::Error::Ptr error) {
            toSerializedError(reply, error);
            reactor->Finish(Status::OK);
        });
    }
    catch (std::exception const& e)
    {
        FRONT_SERVER_LOG(ERROR) << LOG_DESC("onReceiveMessage exception")
                                << LOG_KV("error", boost::diagnostic_information(e));
        toSerializedError(reply,
            std::make_shared<bcos::Error>(-1, std::string(boost::diagnostic_information(e))));
        reactor->Finish(Status::OK);
    }
    return reactor;
}