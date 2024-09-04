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
 * @file RA2018PSIServer.h
 * @author: yujiechen
 * @date 2022-11-8
 */
#pragma once
#include "../RA2018PSIConfig.h"
#include "ppc-framework/io/DataBatch.h"
#include <memory>
namespace ppc::psi
{
class RA2018PSIServer
{
public:
    using Ptr = std::shared_ptr<RA2018PSIServer>;
    RA2018PSIServer(RA2018PSIConfig::Ptr const& _config) : m_config(_config) {}
    virtual ~RA2018PSIServer() = default;

    // the server offline-full-evaluate
    virtual std::vector<bcos::bytes> fullEvaluate(ppc::io::DataBatch::Ptr const& _data);
    // the server evaluate the blindedData
    virtual std::vector<bcos::bytes> evaluate(std::vector<bcos::bytes> const& _blindData);

private:
    RA2018PSIConfig::Ptr m_config;
};
}  // namespace ppc::psi