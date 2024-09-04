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
 * @file FloatingPointCodec.cpp
 * @author: yujiechen
 * @date 2023-08-23
 */
#include "FloatingPointCodec.h"
#include <math.h>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>

using namespace ppc::homo;
using namespace ppc::crypto;
using namespace ppc;
using namespace bcos;

s1024 FloatingPointCodec::fpToS1024(ppc::FloatingPointNumber const& _value)
{
    auto ctx = createBNContext();
    BigNum exponent(_value.exponent);
    auto result = C_BASE.exp(exponent.bn().get(), ctx.get());
    _value.value.mul(result.bn().get(), result.bn().get(), ctx.get());
    return result.toS1024();
}

float50 FloatingPointCodec::toFloat50(ppc::FloatingPointNumber const& _value)
{
    if (_value.exponent >= 0)
    {
        return float50(fpToS1024(_value));
    }
    return toDecimal(_value);
}

float50 FloatingPointCodec::toDecimal(ppc::FloatingPointNumber const& _value)
{
    auto ctx = createBNContext();
    BigNum exponent(-_value.exponent);
    auto divider = C_BASE.exp(exponent.bn().get(), ctx.get());
    return (float50)_value.value.toS1024() / (float50)divider.toS1024();
}

s1024 FloatingPointCodec::toInt(ppc::FloatingPointNumber const& _value)
{
    if (_value.exponent >= 0)
    {
        return fpToS1024(_value);
    }
    else
    {
        return s1024(toDecimal(_value));
    }
}
ppc::FloatingPointNumber FloatingPointCodec::toFloatingPoint(std::string const& _valueStr)
{
    float50 value(_valueStr);
    auto pos = _valueStr.find(".");
    auto exp = 0;
    if (pos != std::string::npos)
    {
        exp = -(_valueStr.size() - 1 - pos);
    }
    // 10^exp
    auto baseExp = boost::multiprecision::pow(s1024(10), -exp);
    // convert the value to int
    auto significant = (s1024)(float50(baseExp) * float50(value));
    FloatingPointNumber result;
    result.exponent = exp;
    // convert the significant to BigNum
    BigNum significantBn(significant);
    result.value = significantBn;
    return result;
}

ppc::FloatingPointNumber FloatingPointCodec::toFloatingPoint(s1024 const& _value)
{
    FloatingPointNumber result;
    BigNum value(_value);
    result.value = value;
    result.exponent = 0;
    return result;
}
