/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file EccCryptoFactoryImpl.h
 * @author: yujiechen
 * @date 2023-1-3
 */
#pragma once
#include "../Common.h"
#include "Ed25519EccCrypto.h"
#include "OpenSSLEccCrypto.h"
#include "ppc-framework/protocol/Protocol.h"

namespace ppc::crypto
{
class EccCryptoFactoryImpl : public EccCryptoFactory
{
public:
    using Ptr = std::shared_ptr<EccCryptoFactoryImpl>;
    EccCryptoFactoryImpl() = default;
    ~EccCryptoFactoryImpl() override = default;

    EccCrypto::Ptr createEccCrypto(int8_t _curveType, Hash::Ptr const& _hashImpl) const override
    {
        switch (_curveType)
        {
        case (int8_t)ppc::protocol::ECCCurve::ED25519:
            return std::make_shared<Ed25519EccCrypto>();
        case (int8_t)ppc::protocol::ECCCurve::SM2:
        case (int8_t)ppc::protocol::ECCCurve::SECP256K1:
        case (int8_t)ppc::protocol::ECCCurve::P256:
            return std::make_shared<OpenSSLEccCrypto>(
                _hashImpl, (ppc::protocol::ECCCurve)_curveType);
        default:
            BOOST_THROW_EXCEPTION(UnsupportedCurveType() << bcos::errinfo_comment(
                                      "unsupported curveType: " + std::to_string(_curveType)));
        }
    }
};
}  // namespace ppc::crypto