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
 * @file MPCInitializer.h
 * @author: caryliao
 * @date 2023-03-24
 */
#pragma once
#include "libinitializer/Common.h"
#include "ppc-rpc/src/RpcFactory.h"
#include <bcos-utilities/BoostLogInitializer.h>
#include <memory>
namespace ppc::rpc
{
class Rpc;
}
namespace ppc::mpc
{
class MPCInitializer
{
public:
    using Ptr = std::shared_ptr<MPCInitializer>;
    MPCInitializer() {}
    virtual ~MPCInitializer() { stop(); }

    virtual void init(std::string const& _configPath);
    virtual void start();
    virtual void stop();

private:
    bcos::BoostLogInitializer::Ptr m_logInitializer;
    std::shared_ptr<ppc::rpc::Rpc> m_rpc;
};
}  // namespace ppc::mpc