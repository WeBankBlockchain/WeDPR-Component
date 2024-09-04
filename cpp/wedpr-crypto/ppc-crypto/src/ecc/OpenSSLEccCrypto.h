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
 * @file OpenSSLEccCrypto.h
 * @author: yujiechen
 * @date 2022-12-5
 */
#pragma once
#include "core/EcPoint.h"
#include "openssl/rand.h"
#include "ppc-framework/crypto/EccCrypto.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/libwrapper/BigNum.h"
namespace ppc::crypto
{
class OpenSSLEccCrypto : public EccCrypto
{
public:
    using Ptr = std::shared_ptr<OpenSSLEccCrypto>;
    OpenSSLEccCrypto(Hash::Ptr _hashImpl, ppc::protocol::ECCCurve const& _curve);
    ~OpenSSLEccCrypto() override = default;

    uint32_t pointSizeInBytes() const override { return m_group.pointBytesLen(); }
    // get the curve
    ppc::protocol::ECCCurve getCurve() const override { return m_curve; }

    // generate a random group-element
    bcos::bytes generateRandomScalar() const override;
    // convert the hashData into ec-point over the given curve
    bcos::bytes hashToCurve(bcos::bytes&& _hashData) const override;
    bcos::bytes hashToCurve(bcos::bytes const& _hashData) const override;

    // get the multiplicative inverse of _data over the given curve
    bcos::bytes scalarInvert(bcos::bytes const& _data) const override;
    // multiply the _ecPoint by _scalar
    bcos::bytes ecMultiply(bcos::bytes const& _ecPoint, bcos::bytes const& _scalar) const override;
    bool isValidEcPoint(bcos::bytes const& _ecPoint) const override;
    bcos::bytes mulGenerator(bcos::bytes const& _scalar) const override;

    bcos::bytes ecAdd(bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const override;
    bcos::bytes ecSub(bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const override;

    bcos::bytes hashToScalar(bcos::bytes const& _hashData) const override;
    bcos::bytes scalarAdd(bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const override;
    bcos::bytes scalarSub(bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const override;
    bcos::bytes scalarMul(bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const override;

    EcGroup const& group() const { return m_group; }
    EcPoint hashToCurveImpl(bcos::bytes const& _hashData) const;
    BigNum generateRandomScalarImpl() const
    {
        bcos::bytes result;
        result.resize(m_keySize);
        if (RAND_bytes(result.data(), m_keySize) != 1)
        {
            BOOST_THROW_EXCEPTION(GenerateRandomScalarError());
        }
        return BigNum(bcos::bytesConstRef(result.data(), result.size()), m_group.n(), false);
    }

    // get the multiplicative inverse of _data over the given curve
    BigNum invertImpl(bcos::bytes const& _data) const
    {
        BigNum value(bcos::bytesConstRef(_data.data(), _data.size()));
        return value.Invert(m_group.n());
    }

    BigNum addImpl(bcos::bytes const& _data1, bcos::bytes const& _data2) const
    {
        BigNum value1(bcos::bytesConstRef(_data1.data(), _data1.size()));
        BigNum value2(bcos::bytesConstRef(_data2.data(), _data2.size()));
        return value1.add(value2, m_group.n());
    }

    BigNum subImpl(bcos::bytes const& _data1, bcos::bytes const& _data2) const
    {
        BigNum value1(bcos::bytesConstRef(_data1.data(), _data1.size()));
        BigNum value2(bcos::bytesConstRef(_data2.data(), _data2.size()));
        return value1.sub(value2, m_group.n());
    }

    BigNum mulImpl(bcos::bytes const& _data1, bcos::bytes const& _data2) const
    {
        BigNum value1(bcos::bytesConstRef(_data1.data(), _data1.size()));
        BigNum value2(bcos::bytesConstRef(_data2.data(), _data2.size()));
        return value1.mul(value2, m_group.n());
    }

private:
    Hash::Ptr m_hashImpl;
    ppc::protocol::ECCCurve m_curve;
    int m_keySize;
    EcGroup m_group;

    constexpr static int SM2_KEY_SIZE = 32;
    constexpr static int SECP256K1_KEY_SIZE = 32;
    constexpr static int P256_KEY_SIZE = 32;

    constexpr static int SM2_POINT_BYTES_LEN = 33;
    constexpr static int SECP256K1_POINT_BYTES_LEN = 33;
    constexpr static int P256_POINT_BYTES_LEN = 33;
};
}  // namespace ppc::crypto