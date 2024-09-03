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
 * @file GrpcClient.cpp
 * @author: yujiechen
 * @date 2024-09-02
 */
#include "GrpcClient.h"
#include "Common.h"


using namespace ppc::protocol;
using namespace grpc;

void GrpcClient::handleRpcResponse()
{
    void* callback;
    bool ok = false;
    // Block until the next result is available in the completion queue "m_queue".
    while (m_queue.Next(&callback, &ok))
    {
        try
        {
            // The tag in this example is the memory location of the call object
            // Note: the should been managed by shared_ptr
            AsyncClientCall* call = static_cast<AsyncClientCall*>(callback);

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            if (!ok)
            {
                GRPC_CLIENT_LOG(WARNING)
                    << LOG_DESC("handleRpcResponse: receive response with unormal status");
                return;
            }
            call->callback(call->context, call->status, std::move(call->reply));
        }
        catch (std::exception const& e)
        {
            GRPC_CLIENT_LOG(WARNING) << LOG_DESC("handleRpcResponse exception")
                                     << LOG_KV("error", boost::diagnostic_information(e));
        }
    }
}