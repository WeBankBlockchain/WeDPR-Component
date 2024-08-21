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
 * @file OreFloatingNumber.h
 * @author: shawnhe
 * @date 2023-12-06
 */
#pragma once

#include "ppc-framework/Common.h"
#include "ppc-tools/src/common/TransTools.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

namespace ppc
{
DERIVE_PPC_EXCEPTION(OreFloatingNumberError);

struct OreFloatingNumber
{
    int64_t integerPart;
    std::string decimalPart;

    OreFloatingNumber() = default;
    virtual ~OreFloatingNumber() = default;

    OreFloatingNumber(const float50& _value)
    {
        if (_value > (std::numeric_limits<int64_t>::max)() / 2 - 1 ||
            _value <= (std::numeric_limits<int64_t>::min)() / 2 + 1)
        {
            BOOST_THROW_EXCEPTION(
                OreFloatingNumberError() << bcos::errinfo_comment(
                    "plain is too large or too small, must be in range (-2^62 + 1, 2^62 - 1)"));
        }
        // set to positive
        // Note: If there are too many decimal, there will be a problem of precision truncation.
        float50 value = _value + (std::numeric_limits<int64_t>::max)() / 2;
        std::string valueStr = value.str();
        auto dotPos = valueStr.find('.');
        int64_t intValue = std::stoll(valueStr.substr(0, dotPos));

        integerPart = tools::toBigEndian(intValue);

        if (dotPos != std::string::npos)
        {
            decimalPart = valueStr.substr(dotPos + 1, valueStr.size());
        }
    }

    float50 value()
    {
        std::string valueStr;
        if (decimalPart.empty())
        {
            valueStr = std::to_string(ppc::tools::fromBigEndian(integerPart));
        }
        else
        {
            valueStr = std::to_string(ppc::tools::fromBigEndian(integerPart)) + "." + decimalPart;
        }

        return float50(valueStr) - (std::numeric_limits<int64_t>::max)() / 2;
    }
};
}  // namespace ppc
