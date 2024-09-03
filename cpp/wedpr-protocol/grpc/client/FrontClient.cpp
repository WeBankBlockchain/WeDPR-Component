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
 * @file FrontClient.cpp
 * @author: yujiechen
 * @date 2024-09-02
 */
#include "FrontClient.h"
#include "protobuf/RequestConverter.h"
#include "wedpr-protocol/protobuf/Common.h"


using namespace ppc::protocol;
using namespace ppc::proto;
using namespace grpc;

void FrontClient::onReceiveMessage(ppc::protocol::Message::Ptr const& msg, ReceiveMsgFunc callback)
{
    // TODO: optimize here
    ReceivedMessage receivedMsg;
    bcos::bytes encodedData;
    msg->encode(encodedData);
    receivedMsg.set_data(encodedData.data(), encodedData.size());

    ClientContext context;
    auto response = std::make_shared<Error>();
    m_stub->async()->onReceiveMessage(&context, &receivedMsg, response.get(),
        [response, callback](Status status) { callback(toError(status, std::move(*response))); });
}