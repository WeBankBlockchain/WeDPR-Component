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
using namespace ppc::proto;
using namespace grpc;
using namespace grpc::health::v1;

bool GrpcClient::checkHealth()
{
    try
    {
        ClientContext context;
        HealthCheckResponse response;
        auto status =
            m_healthCheckStub->Check(&context, HealthCheckRequest::default_instance(), &response);
        if (!status.ok())
        {
            GRPC_CLIENT_LOG(WARNING)
                << LOG_DESC("GrpcClient check health failed") << LOG_KV("code", status.error_code())
                << LOG_KV("msg", status.error_message());
            return false;
        }
        return true;
    }
    catch (std::exception const& e)
    {
        GRPC_CLIENT_LOG(WARNING) << LOG_DESC("GrpcClient check health exception")
                                 << LOG_KV("error", boost::diagnostic_information(e));
        return false;
    }
}