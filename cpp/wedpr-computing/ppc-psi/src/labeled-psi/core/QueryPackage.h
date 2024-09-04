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
 * @file QueryPackage.h
 * @author: shawnhe
 * @date 2022-11-14
 *
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <apsi/powers.h>
#include <seal/ciphertext.h>
#include <seal/relinkeys.h>

#include "LabeledPSI.h"
#include "SenderDB.h"
#include "ppc-psi/src/labeled-psi/Common.h"


namespace ppc::psi
{
class QueryPackage
{
public:
    QueryPackage() = default;

    QueryPackage(ppctars::QueryRequest _queryQuest, SenderDB::Ptr _senderDB);

    const seal::RelinKeys& relinKeys() { return m_relinKeys; }

    const std::unordered_map<std::uint32_t, std::vector<seal::Ciphertext>>& powers()
    {
        return m_powers;
    }

    const apsi::PowersDag& powersDag() const noexcept { return m_powersDag; }

private:
    seal::RelinKeys m_relinKeys;

    std::unordered_map<std::uint32_t, std::vector<seal::Ciphertext>> m_powers;

    apsi::PowersDag m_powersDag;
};

}  // namespace ppc::psi
