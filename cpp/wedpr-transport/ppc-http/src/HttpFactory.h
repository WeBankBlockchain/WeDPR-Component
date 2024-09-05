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
 * @file HttpFactory.h
 * @author: zachma
 * @date 2023-8-23
 */
#pragma once
#include "Http.h"
#include <memory>

namespace bcos::boostssl::ws
{
class WsConfig;
}
namespace ppc::tools
{
class PPCConfig;
}
namespace ppc::http
{
class HttpFactory
{
public:
    using Ptr = std::shared_ptr<HttpFactory>;
    HttpFactory(std::string const& _selfPartyID) : m_selfPartyID(_selfPartyID) {}
    virtual ~HttpFactory() = default;

    Http::Ptr buildHttp(std::shared_ptr<ppc::tools::PPCConfig const> _config);

private:
    std::shared_ptr<bcos::boostssl::ws::WsConfig> initConfig(
        std::shared_ptr<ppc::tools::PPCConfig const> _config);

private:
    std::string m_selfPartyID;
};
}  // namespace ppc::http