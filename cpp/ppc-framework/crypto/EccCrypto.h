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
 * @file EccCrypto.h
 * @author: yujiechen
 * @date 2022-11-1
 */
#pragma once
#include "../protocol/Protocol.h"
#include "Hash.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::crypto
{
class EccCrypto
{
public:
    using Ptr = std::shared_ptr<EccCrypto>;
    EccCrypto() = default;
    virtual ~EccCrypto() {}

    virtual uint32_t pointSizeInBytes() const = 0;
    // get the curve
    virtual ppc::protocol::ECCCurve getCurve() const = 0;
    // generate a random group-element
    virtual bcos::bytes generateRandomScalar() const = 0;
    // convert the hashData into ec-point over the given curve
    virtual bcos::bytes hashToCurve(bcos::bytes&& _hashData) const = 0;
    virtual bcos::bytes hashToCurve(bcos::bytes const& _hashData) const = 0;
    virtual bcos::bytes hashToScalar(bcos::bytes const& _hashData) const = 0;

    // get the multiplicative inverse of _data over the given curve
    virtual bcos::bytes scalarInvert(bcos::bytes const& _data) const = 0;
    virtual bcos::bytes scalarAdd(bcos::bytes const& _data1, bcos::bytes const& _data2) const = 0;
    virtual bcos::bytes scalarSub(bcos::bytes const& _data1, bcos::bytes const& _data2) const = 0;
    virtual bcos::bytes scalarMul(bcos::bytes const& _data1, bcos::bytes const& _data2) const = 0;
    // multiply the _ecPoint by _scalar
    virtual bcos::bytes ecMultiply(
        bcos::bytes const& _ecPoint, bcos::bytes const& _scalar) const = 0;
    virtual bcos::bytes ecAdd(bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const = 0;
    virtual bcos::bytes ecSub(bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const = 0;
    virtual bool isValidEcPoint(bcos::bytes const& _ecPoint) const = 0;
    virtual bcos::bytes mulGenerator(bcos::bytes const& _scalar) const = 0;
};

class EccCryptoFactory
{
public:
    using Ptr = std::shared_ptr<EccCryptoFactory>;
    EccCryptoFactory() = default;
    virtual ~EccCryptoFactory() = default;

    virtual EccCrypto::Ptr createEccCrypto(int8_t _curveType, Hash::Ptr const& _hashImpl) const = 0;
};
}  // namespace ppc::crypto
