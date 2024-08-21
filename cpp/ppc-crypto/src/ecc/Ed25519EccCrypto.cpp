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
 * @file Ed25519EccCrypto.cpp
 * @author: yujiechen
 * @date 2022-11-2
 */
#include "Ed25519EccCrypto.h"
#include "../Common.h"
#include <sodium.h>
using namespace ppc::crypto;
using namespace bcos;

uint32_t Ed25519EccCrypto::pointSizeInBytes() const
{
    return crypto_core_ed25519_BYTES;
}

bcos::bytes Ed25519EccCrypto::generateRandomScalar() const
{
    bcos::bytes scalar(crypto_core_ed25519_SCALARBYTES);
    crypto_core_ed25519_random(scalar.data());
    bcos::bytes reducedResult(crypto_core_ed25519_SCALARBYTES);
    // Note: must reduce here to mod L
    crypto_core_ed25519_scalar_reduce(reducedResult.data(), scalar.data());
    return reducedResult;
}

bcos::bytes Ed25519EccCrypto::hashToCurve(bcos::bytes&& _hashData) const
{
    if (_hashData.size() < crypto_core_ed25519_HASHBYTES)
    {
        _hashData.resize(crypto_core_ed25519_HASHBYTES);
    }
    return convertHashToCurve(_hashData);
}

bcos::bytes Ed25519EccCrypto::hashToCurve(bcos::bytes const& _hashData) const
{
    if (_hashData.size() < crypto_core_ed25519_HASHBYTES)
    {
        bcos::bytes tmpData(_hashData.begin(), _hashData.end());
        tmpData.resize(crypto_core_ed25519_HASHBYTES);
        return convertHashToCurve(tmpData);
    }
    return convertHashToCurve(_hashData);
}

// convert the hashData into ec-point over the given curve
bcos::bytes Ed25519EccCrypto::convertHashToCurve(bcos::bytes const& _hashData) const
{
    bcos::bytes ecPoint(crypto_core_ed25519_BYTES);
    auto ret = crypto_core_ed25519_from_hash(ecPoint.data(), _hashData.data());
    if (ret)
    {
        BOOST_THROW_EXCEPTION(
            HashToCurveException() << errinfo_comment(
                "call crypto_core_ed25519_from_hash error, code: " + std::to_string(ret)));
    }
    return ecPoint;
}

// get the multiplicative inverse of _data over the given curve
bcos::bytes Ed25519EccCrypto::scalarInvert(bcos::bytes const& _data) const
{
    if (_data.size() < crypto_core_ed25519_SCALARBYTES)
    {
        BOOST_THROW_EXCEPTION(ScalarInvertException() << errinfo_comment(
                                  "the length of scalar-data must be no smaller than: " +
                                  std::to_string(crypto_core_ed25519_SCALARBYTES)));
    }
    bcos::bytes invertedScalar(crypto_core_ed25519_SCALARBYTES);
    auto ret = crypto_core_ed25519_scalar_invert(invertedScalar.data(), _data.data());
    if (ret)
    {
        BOOST_THROW_EXCEPTION(
            HashToCurveException() << errinfo_comment(
                "call crypto_core_ed25519_scalar_invert error, code: " + std::to_string(ret)));
    }
    return invertedScalar;
}

// multiply the _ecPoint by _scalar
bcos::bytes Ed25519EccCrypto::ecMultiply(
    bcos::bytes const& _ecPoint, bcos::bytes const& _scalar) const
{
    if (_ecPoint.size() < crypto_scalarmult_ed25519_BYTES)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "the ecpoint for ed25519 must be no smaller than: " +
                                  std::to_string(crypto_scalarmult_ed25519_BYTES)));
    }
    if (_scalar.size() < crypto_scalarmult_ed25519_SCALARBYTES)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "the scalar for ed25519 must be no smaller than: " +
                                  std::to_string(crypto_scalarmult_ed25519_SCALARBYTES)));
    }
    bcos::bytes result(crypto_scalarmult_ed25519_BYTES);
    auto ret = crypto_scalarmult_ed25519_noclamp(result.data(), _scalar.data(), _ecPoint.data());
    if (ret)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "crypto_scalarmult_ed25519 error: " + std::to_string(ret)));
    }
    return result;
}

bcos::bytes Ed25519EccCrypto::ecAdd(
    bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const
{
    if (_ecPointA.size() < crypto_scalarmult_ed25519_BYTES ||
        _ecPointB.size() < crypto_scalarmult_ed25519_BYTES)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "the ecpoint for ed25519 must be no smaller than: " +
                                  std::to_string(crypto_scalarmult_ed25519_BYTES)));
    }

    bcos::bytes result(crypto_scalarmult_ed25519_BYTES);
    auto ret = crypto_core_ed25519_add(result.data(), _ecPointA.data(), _ecPointB.data());
    if (ret)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "crypto_core_ed25519_add error: " + std::to_string(ret)));
    }
    return result;
}

bcos::bytes Ed25519EccCrypto::ecSub(
    bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const
{
    if (_ecPointA.size() < crypto_scalarmult_ed25519_BYTES ||
        _ecPointB.size() < crypto_scalarmult_ed25519_BYTES)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "the ecpoint for ed25519 must be no smaller than: " +
                                  std::to_string(crypto_scalarmult_ed25519_BYTES)));
    }

    bcos::bytes result(crypto_scalarmult_ed25519_BYTES);
    auto ret = crypto_core_ed25519_sub(result.data(), _ecPointA.data(), _ecPointB.data());
    if (ret)
    {
        BOOST_THROW_EXCEPTION(EcMultipleException() << errinfo_comment(
                                  "crypto_core_ed25519_sub error: " + std::to_string(ret)));
    }
    return result;
}

bool Ed25519EccCrypto::isValidEcPoint(bcos::bytes const& _ecPoint) const
{
    if (_ecPoint.size() < crypto_core_ed25519_SCALARBYTES)
    {
        return false;
    }
    // Note: if _ecPoint is not a valid point, return 0
    return (crypto_core_ed25519_is_valid_point(_ecPoint.data()) != 0);
}

bcos::bytes Ed25519EccCrypto::mulGenerator(bcos::bytes const& _scalar) const
{
    bcos::bytes result(crypto_scalarmult_ed25519_SCALARBYTES);
    auto ret = crypto_scalarmult_ed25519_base_noclamp(result.data(), _scalar.data());
    if (ret)
    {
        BOOST_THROW_EXCEPTION(
            EcMultipleException() << errinfo_comment(
                "crypto_scalarmult_ed25519_base_noclamp error: " + std::to_string(ret)));
    }
    return result;
}

bcos::bytes Ed25519EccCrypto::hashToScalar(bcos::bytes const& _hashData) const
{
    BOOST_THROW_EXCEPTION(UnsupportedCurveType() << errinfo_comment("method not support: "));
}

bcos::bytes Ed25519EccCrypto::scalarAdd(
    bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const
{
    BOOST_THROW_EXCEPTION(UnsupportedCurveType() << errinfo_comment("method not support: "));
}

bcos::bytes Ed25519EccCrypto::scalarSub(
    bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const
{
    BOOST_THROW_EXCEPTION(UnsupportedCurveType() << errinfo_comment("method not support: "));
}

bcos::bytes Ed25519EccCrypto::scalarMul(
    bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const
{
    BOOST_THROW_EXCEPTION(UnsupportedCurveType() << errinfo_comment("method not support: "));
}
