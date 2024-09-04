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
 * @file FrontClient.h
 * @author: yujiechen
 * @date 2024-09-02
 */
#pragma once
#include "GrpcClient.h"
#include "ppc-framework/front/IFront.h"

namespace ppc::protocol
{
class FrontClient : public virtual ppc::front::IFrontClient, public GrpcClient
{
public:
    using Ptr = std::shared_ptr<FrontClient>;
    FrontClient(ppc::protocol::GrpcConfig::Ptr const& grpcConfig, std::string const& endPoints)
      : GrpcClient(grpcConfig, endPoints), m_stub(ppc::proto::Front::NewStub(m_channel))
    {}

    ~FrontClient() override = default;
    void onReceiveMessage(
        ppc::protocol::Message::Ptr const& _msg, ppc::protocol::ReceiveMsgFunc _callback) override;

private:
    std::unique_ptr<ppc::proto::Front::Stub> m_stub;
};
}  // namespace ppc::protocol