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
 * @file FloatingPointCodec.h
 * @author: yujiechen
 * @date 2023-08-23
 */
#pragma once
#include <ppc-framework/libwrapper/BigNum.h>
#include <ppc-framework/libwrapper/FloatingPointNumber.h>
#include <memory>
namespace ppc::homo
{
DERIVE_PPC_EXCEPTION(FloatingPointCodecException);

class FloatingPointCodec
{
public:
    using Ptr = std::shared_ptr<FloatingPointCodec>;
    FloatingPointCodec() = default;
    virtual ~FloatingPointCodec() = default;
    s1024 toInt(ppc::FloatingPointNumber const& _value);
    float50 toFloat50(ppc::FloatingPointNumber const& _value);

    ppc::FloatingPointNumber toFloatingPoint(std::string const& _value);
    ppc::FloatingPointNumber toFloatingPoint(s1024 const& _value);

private:
    s1024 fpToS1024(ppc::FloatingPointNumber const& _value);
    float50 toDecimal(ppc::FloatingPointNumber const& _value);

private:
    const ppc::crypto::BigNum C_BASE = ppc::crypto::BigNum(10);
};
}  // namespace ppc::homo