/*
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
 * @file RpcFactory.h
 * @author: yujiechen
 * @date 2022-11-4
 */
#pragma once
#include "Rpc.h"
#include "ppc-tools/src/config/PPCConfig.h"
#include <memory>
namespace bcos::boostssl::ws
{
class WsConfig;
}
namespace ppc::rpc
{
class RpcFactory
{
public:
    using Ptr = std::shared_ptr<RpcFactory>;
    RpcFactory(std::string const& _selfPartyID) : m_selfPartyID(_selfPartyID) {}
    virtual ~RpcFactory() = default;

    Rpc::Ptr buildRpc(ppc::tools::PPCConfig::ConstPtr _config);


private:
    std::shared_ptr<bcos::boostssl::ws::WsConfig> initConfig(
        ppc::tools::PPCConfig::ConstPtr _config);

private:
    std::string m_selfPartyID;
};
}  // namespace ppc::rpc