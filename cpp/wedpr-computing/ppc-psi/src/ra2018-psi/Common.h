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
 * @file Common.h
 * @author: yujiechen
 * @date 2022-10-27
 */
#pragma once
#include "../Common.h"
#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-tools/src/cuckoo/Cuckoofilter.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>

#define RA2018_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PSI: RA2018")

namespace ppc::psi
{
using DefaultCukooFilter =
    ppc::tools::Cuckoofilter<ppc::crypto::BitMixMurmurHash, ppc::tools::HashTable>;
using DefaultCukooFilterPtr =
    std::shared_ptr<ppc::tools::Cuckoofilter<ppc::crypto::BitMixMurmurHash, ppc::tools::HashTable>>;
// the data-preprocessing command
constexpr static std::string_view DATA_PREPROCESSING_CMD = "data_preprocessing";
// the psi command
constexpr static std::string_view RUN_PSI_CMD = "ra2018_psi";

enum DataPreProcessingOption : int
{
    Insert,
    Delete,
};
inline std::ostream& operator<<(std::ostream& _out, DataPreProcessingOption const& _option)
{
    switch (_option)
    {
    case DataPreProcessingOption::Insert:
        _out << "Insert";
        break;
    case DataPreProcessingOption::Delete:
        _out << "Delete";
        break;
    default:
        _out << "UnknownDataPreProcessingOption";
        break;
    }
    return _out;
}

enum RA2018Command : int
{
    DATA_PREPROCESSING,
    RUN_PSI,
};
inline std::ostream& operator<<(std::ostream& _out, RA2018Command const& _command)
{
    switch (_command)
    {
    case RA2018Command::DATA_PREPROCESSING:
        _out << "DATA_PREPROCESSING";
        break;
    case RA2018Command::RUN_PSI:
        _out << "RUN_PSI";
        break;
    default:
        _out << "UnknownRA2018PSICommand";
        break;
    }
    return _out;
}
}  // namespace ppc::psi