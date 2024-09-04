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
 * @file OpenSSLEccCrypto.cpp
 * @author: yujiechen
 * @date 2022-12-5
 */
#include "OpenSSLEccCrypto.h"
#include "openssl/obj_mac.h"
#include "ppc-framework/libwrapper/BigNum.h"
using namespace ppc::crypto;
using namespace ppc::protocol;
using namespace bcos;

OpenSSLEccCrypto::OpenSSLEccCrypto(Hash::Ptr _hashImpl, ppc::protocol::ECCCurve const& _curve)
  : m_hashImpl(std::move(_hashImpl)), m_curve(_curve)
{
    switch (_curve)
    {
    case ECCCurve::SM2:
    {
        m_group = EcGroup(SM2_POINT_BYTES_LEN, NID_sm2);
        m_keySize = SM2_KEY_SIZE;
        break;
    }
    case ECCCurve::SECP256K1:
    {
        m_group = EcGroup(SECP256K1_POINT_BYTES_LEN, NID_secp256k1);
        m_keySize = SECP256K1_KEY_SIZE;
        break;
    }
    case ECCCurve::P256:
    {
        m_group = EcGroup(P256_POINT_BYTES_LEN, NID_X9_62_prime256v1);
        m_keySize = P256_KEY_SIZE;
        break;
    }
    default:
    {
        BOOST_THROW_EXCEPTION(UnsupportedCurveType() << errinfo_comment(
                                  "Not supported curve type " + std::to_string((int)_curve)));
    }
    }
}

// generate a random group-element
bcos::bytes OpenSSLEccCrypto::generateRandomScalar() const
{
    bcos::bytes result;
    generateRandomScalarImpl().toBytes(result);
    return result;
}


// convert the hashData into ec-point over the given curve
bcos::bytes OpenSSLEccCrypto::hashToCurve(bcos::bytes&& _hashData) const
{
    bcos::bytes result;
    hashToCurveImpl(_hashData).toBytes(result);
    return result;
}

bcos::bytes OpenSSLEccCrypto::hashToCurve(bcos::bytes const& _hashData) const
{
    bcos::bytes result;
    hashToCurveImpl(_hashData).toBytes(result);
    return result;
}

// get the multiplicative inverse of _data over the given curve
bcos::bytes OpenSSLEccCrypto::scalarInvert(bcos::bytes const& _data) const
{
    try
    {
        bcos::bytes result;
        invertImpl(_data).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(ScalarInvertException()
                              << errinfo_comment("OpenSSLEccCrypto scalarInvert error: " +
                                                 std::string(boost::diagnostic_information(e))));
    }
}


bcos::bytes OpenSSLEccCrypto::scalarAdd(
    bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const
{
    try
    {
        bcos::bytes result;
        result.resize(_scalarA.size());
        addImpl(_scalarA, _scalarB).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(ScalarCalculateException()
                              << errinfo_comment("OpenSSLEccCrypto scalarAdd error: " +
                                                 std::string(boost::diagnostic_information(e))));
    }
}

bcos::bytes OpenSSLEccCrypto::scalarSub(
    bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const
{
    try
    {
        bcos::bytes result;
        result.resize(_scalarA.size());
        subImpl(_scalarA, _scalarB).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(ScalarCalculateException()
                              << errinfo_comment("OpenSSLEccCrypto scalarSub error: " +
                                                 std::string(boost::diagnostic_information(e))));
    }
}


bcos::bytes OpenSSLEccCrypto::scalarMul(
    bcos::bytes const& _scalarA, bcos::bytes const& _scalarB) const
{
    try
    {
        bcos::bytes result;
        result.resize(_scalarA.size());
        mulImpl(_scalarA, _scalarB).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(ScalarCalculateException()
                              << errinfo_comment("OpenSSLEccCrypto scalscalarMularSub error: " +
                                                 std::string(boost::diagnostic_information(e))));
    }
}

bcos::bytes OpenSSLEccCrypto::hashToScalar(const bcos::bytes& _hashData) const
{
    try
    {
        BigNum x;
        auto hashResult = m_hashImpl->hash(ref(_hashData));
        x.fromBytesModP(ref(hashResult), m_group.p());
        bcos::bytes result;
        result.resize(hashResult.size());
        x.toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(HashToScalarException()
                              << errinfo_comment("OpenSSLEccCrypto hashToScalar error: " +
                                                 std::string(boost::diagnostic_information(e))));
    }
}


EcPoint OpenSSLEccCrypto::hashToCurveImpl(bcos::bytes const& _hashData) const
{
    try
    {
        auto bnContext = createBNContext();
        BigNum x;
        // Note: After actual measurement, sm2 will always retry once, so for the sm2 elliptic
        // curve, will hash the hashData again
        if (m_curve == ECCCurve::SM2)
        {
            auto hashResult = m_hashImpl->hash(ref(_hashData));
            x.fromBytesModP(ref(hashResult), m_group.p());
        }
        else
        {
            x.fromBytesModP(ref(_hashData), m_group.p());
        }
        int retryCount = 0;
        EcPoint point(m_group);
        while (true)
        {
            if (EC_POINT_set_compressed_coordinates(m_group.ecGroup().get(), point.point().get(),
                    x.bn().get(), 0, bnContext.get()) == 1)
            {
                break;
            }
            // re-hash the x
            bcos::bytes xBytes;
            x.toBytes(xBytes);
            auto hashResult = m_hashImpl->hash(ref(xBytes));
            x.fromBytesModP(ref(hashResult), m_group.p());
            retryCount++;
        }
        return point;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(HashToCurveException()
                              << errinfo_comment("OpenSSLEccCrypto hashToCurve error: " +
                                                 std::string(boost::diagnostic_information(e))));
    }
}

// multiply the _ecPoint by _scalar
bcos::bytes OpenSSLEccCrypto::ecMultiply(
    bcos::bytes const& _ecPoint, bcos::bytes const& _scalar) const
{
    try
    {
        // convert bytes to EcPoint
        EcPoint point(m_group, ref(_ecPoint));
        // convert bytes to BigNumber
        BigNum sk(ref(_scalar));
        bcos::bytes result;
        point.ecMultiply(sk).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(
            EcMultipleException() << errinfo_comment(
                "ecMultiply error:" + std::string(boost::diagnostic_information(e))));
    }
}

bool OpenSSLEccCrypto::isValidEcPoint(bcos::bytes const& _ecPoint) const
{
    // try to convert to EcPoint
    try
    {
        EcPoint point(m_group, ref(_ecPoint));
        return true;
    }
    catch (std::exception const& e)
    {
        CRYPTO_LOG(WARNING) << LOG_DESC("isValidEcPoint: convert bytes to ecPoint error")
                            << LOG_KV("exception", boost::diagnostic_information(e));
        return false;
    }
}

bcos::bytes OpenSSLEccCrypto::mulGenerator(bcos::bytes const& _scalar) const
{
    try
    {
        BigNum sk(ref(_scalar));
        bcos::bytes result;
        basePointMultiply(m_group, sk).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(
            MulGeneratorError() << errinfo_comment("OpenSSLEccCrypto mulGenerator error: " +
                                                   std::string(boost::diagnostic_information(e))));
    }
}

bcos::bytes OpenSSLEccCrypto::ecAdd(
    bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const
{
    try
    {
        EcPoint pointA(m_group, ref(_ecPointA));
        EcPoint pointB(m_group, ref(_ecPointB));
        bcos::bytes result;
        pointA.add(pointB).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(
            EcAddError() << errinfo_comment(
                "OpenSSLEccCrypto ecAdd error: " + std::string(boost::diagnostic_information(e))));
    }
}
bcos::bytes OpenSSLEccCrypto::ecSub(
    bcos::bytes const& _ecPointA, bcos::bytes const& _ecPointB) const
{
    try
    {
        EcPoint pointA(m_group, ref(_ecPointA));
        EcPoint pointB(m_group, ref(_ecPointB));
        bcos::bytes result;
        pointA.sub(pointB).toBytes(result);
        return result;
    }
    catch (std::exception const& e)
    {
        BOOST_THROW_EXCEPTION(
            EcSubError() << errinfo_comment(
                "OpenSSLEccCrypto ecSub error: " + std::string(boost::diagnostic_information(e))));
    }
}