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
#include "wedpr-protocol/protobuf/Common.h"


using namespace ppc::protocol;
using namespace ppc::proto;
using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

void FrontClient::onReceiveMessage(ppc::protocol::Message::Ptr const& msg, ReceiveMsgFunc callback)
{
    // TODO: optimize here
    ReceivedMessage receivedMsg;
    bcos::bytes encodedData;
    msg->encode(encodedData);
    receivedMsg.set_data(encodedData.data(), encodedData.size());

    auto grpcCallback = [callback](ClientContext const&, Status const& status, Error&& response) {
        auto error = std::make_shared<bcos::Error>(response.errorcode(), response.errormessage());
        callback(error);
    };

    auto call = std::make_shared<AsyncClientCall>(grpcCallback);
    call->responseReader =
        m_stub->PrepareAsynconReceiveMessage(&call->context, receivedMsg, &m_client->queue());
    call->responseReader->StartCall();
    // send request, upon completion of the RPC, "reply" be updated with the server's response
    call->responseReader->Finish(&call->reply, &call->status, (void*)call.get());
}