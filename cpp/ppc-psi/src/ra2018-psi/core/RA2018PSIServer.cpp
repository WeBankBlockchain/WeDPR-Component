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
 * @file RA2018PSIServer.cpp
 * @author: yujiechen
 * @date 2022-11-8
 */
#include "RA2018PSIServer.h"
#include <bcos-utilities/Common.h>

using namespace ppc::psi;
using namespace ppc::io;
using namespace bcos;

// full-evaluate the input data
std::vector<bcos::bytes> RA2018PSIServer::fullEvaluate(DataBatch::Ptr const& _data)
{
    auto startT = utcSteadyTime();
    std::vector<bcos::bytes> result;
    m_config->oprf()->fullEvaluate(_data, result);
    RA2018_LOG(INFO) << LOG_DESC("RA2018PSIServer fullEvaluate finish")
                     << LOG_KV("timecost", (utcSteadyTime() - startT))
                     << LOG_KV("dataSize", _data->size());
    return result;
}

std::vector<bcos::bytes> RA2018PSIServer::evaluate(std::vector<bcos::bytes> const& _blindData)
{
    auto startT = utcSteadyTime();
    std::vector<bcos::bytes> result;
    m_config->oprf()->evaluate(_blindData, result);
    RA2018_LOG(INFO) << LOG_DESC("RA2018PSIServer evaluate finish")
                     << LOG_KV("timecost", (utcSteadyTime() - startT))
                     << LOG_KV("dataSize", _blindData.size());
    return result;
}