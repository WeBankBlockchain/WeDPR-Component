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
 * @file Ed25519EccCrypto.h
 * @author: yujiechen
 * @date 2022-11-2
 */
#pragma once
#include "ppc-framework/crypto/EccCrypto.h"
#include "ppc-framework/protocol/Protocol.h"

namespace ppc::crypto
{
class Ed25519EccCrypto : public EccCrypto
{
public:
    using Ptr = std::shared_ptr<Ed25519EccCrypto>;
    Ed25519EccCrypto() = default;
    ~Ed25519EccCrypto() override = default;

    uint32_t pointSizeInBytes() const override;
    // get the curve
    ppc::protocol::ECCCurve getCurve() const override { return ppc::protocol::ECCCurve::ED25519; }

    bcos::bytes generateRandomScalar() const override;
    // convert the hashData into ec-point over the given curve
    bcos::bytes hashToCurve(bcos::bytes&& _hashData) const override;
    bcos::bytes hashToCurve(bcos::bytes const& _hashData) const override;
    // get the multiplicative inverse of _data over the given curve
    bcos::bytes scalarInvert(bcos::bytes const& _data) const override;
    // multiply the _ecPoint by _scalar
    bcos::bytes ecMultiply(bcos::bytes const& _ecPoint, bcos::bytes const& _scalar) const override;
    bcos::bytes ecAdd(bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const override;
    bcos::bytes ecSub(bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const override;
    bool isValidEcPoint(bcos::bytes const& _ecPoint) const override;
    bcos::bytes mulGenerator(bcos::bytes const& _scalar) const override;

    bcos::bytes hashToScalar(bcos::bytes const& _hashData) const override;
    bcos::bytes scalarAdd(bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const override;
    bcos::bytes scalarSub(bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const override;
    bcos::bytes scalarMul(bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const override;


private:
    bcos::bytes convertHashToCurve(bcos::bytes const& _hashData) const;
};
}  // namespace ppc::crypto