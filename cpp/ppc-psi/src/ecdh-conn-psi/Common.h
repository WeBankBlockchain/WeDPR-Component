/**
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
 * @file Common.h
 * @author: zachma
 * @date 2023-8-28
 */

#pragma once
#include "ppc-framework/Common.h"
#include <string.h>

namespace ppc::psi
{
#define ECDH_CONN_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("ECDH-CONN-PSI")
DERIVE_PPC_EXCEPTION(ECDHCONNException);

enum class EcdhConnProcess : int8_t
{
    HandShakeProcess = 1,
    CipherProcess = 2,
    END = 3,
};

enum class EcdhConnSubProcess : int8_t
{
    CipherSecondProcess = 1
};

const std::string VERSION_HEAD = "x-ptp-version";
const std::string TECH_PROVIDER_CODE_HEAD = "x-ptp-tech-provider-code";
const std::string TRACE_ID_HEAD = "x-ptp-trace-id";
const std::string TOKEN_HEAD = "x-ptp-token";
const std::string SESSION_ID_HEAD = "x-ptp-session-id";
const std::string TOPIC_HEAD = "x-ptp-topic";

const std::string SOURCE_NODE_HEAD = "x-ptp-source-node-id";
const std::string SOURCE_INST_HEAD = "x-ptp-source-inst-id";
const std::string TARGET_NODE_HEAD = "x-ptp-target-node-id";
const std::string TARGET_INST_HEAD = "x-ptp-target-inst-id";

inline std::ostream& operator<<(std::ostream& _out, EcdhConnProcess const& _type)
{
    switch (_type)
    {
    case EcdhConnProcess::HandShakeProcess:
        _out << "HandShakeProcess";
        break;
    case EcdhConnProcess::CipherProcess:
        _out << "CipherProcess";
        break;
    default:
        _out << "UnknownTaskType";
        break;
    }
    return _out;
}

inline std::ostream& operator<<(std::ostream& _out, EcdhConnSubProcess const& _type)
{
    switch (_type)
    {
    case EcdhConnSubProcess::CipherSecondProcess:
        _out << "CipherSecondProcess";
        break;
    default:
        _out << "UnknownTaskType";
        break;
    }
    return _out;
}
}  // namespace ppc::psi