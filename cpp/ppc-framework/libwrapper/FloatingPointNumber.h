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
 * @file FloatingPointNumber.h
 * @author: yujiechen
 * @date 2023-08-16
 */
#pragma once
#include "BigNum.h"
#include "openssl/bn.h"
#include <openssl/ossl_typ.h>
#include <stdint.h>
#include <memory>
namespace ppc
{
DERIVE_PPC_EXCEPTION(FloatingPointNumberError);
struct FloatingPointNumber
{
    ppc::crypto::BigNum value;
    int16_t exponent;

    using Ptr = std::shared_ptr<FloatingPointNumber>;
    FloatingPointNumber() = default;
    FloatingPointNumber(ppc::crypto::BigNum&& _value, int _exponent)
      : value(std::move(_value)), exponent(_exponent)
    {}
    virtual ~FloatingPointNumber() = default;
};
}  // namespace ppc