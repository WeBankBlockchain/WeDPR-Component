/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file TestEccCrypto.cpp
 * @author: yujiechen
 * @date 2022-11-2
 */
#include "ppc-crypto-core/src/hash/SM3Hash.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto-core/src/hash/Sha512Hash.h"
#include "ppc-crypto/src/Common.h"
#include "ppc-crypto/src/ecc/ECDHCryptoImpl.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-crypto/src/ecc/IppECDHCryptoImpl.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-framework/io/DataBatch.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::protocol;
using namespace bcos;
using namespace ppc::io;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(eccCryptoTest, TestPromptFixture)

void testOpenSSLEccCrypto(OpenSSLEccCrypto::Ptr _eccCryptoImpl, Hash::Ptr _hashImpl)
{
    std::string inputStr = "wer234lkejskdf";
    bcos::bytes inputData(inputStr.begin(), inputStr.end());
    auto hashResult = _hashImpl->hash(ref(inputData));

    auto ecPointBytes = _eccCryptoImpl->hashToCurve(hashResult);

    EcPoint point(_eccCryptoImpl->group(), ref(ecPointBytes));
    bcos::bytes result;
    point.toBytes(result);
    BOOST_CHECK(ecPointBytes == result);

    EcPoint decodedPoint(_eccCryptoImpl->group(), ref(result));
    bcos::bytes result2;
    decodedPoint.toBytes(result2);
    BOOST_CHECK(result == result2);
}

void testEccCrypto(
    EccCrypto::Ptr _eccCryptoImpl, Hash::Ptr _hashImpl, bool _ed25519, bool _testIppCrypto)
{
    std::string inputStr = "wer234lkejskdf";
    bcos::bytes inputData(inputStr.begin(), inputStr.end());
    auto hashResult = _hashImpl->hash(ref(inputData));

    // test hashToCurve
    auto randomScalar = _eccCryptoImpl->generateRandomScalar();
    auto invRandomScalar = _eccCryptoImpl->scalarInvert(randomScalar);
    // check the scalarInvert value
    for (int i = 0; i < 1000; i++)
    {
        BOOST_CHECK(invRandomScalar == _eccCryptoImpl->scalarInvert(randomScalar));
    }

    // scalarInvert the invRandomScalar
    std::cout << "#### randomScalar: " << *toHexString(randomScalar) << std::endl;
    std::cout << "#### invRandomScalar : " << *toHexString(invRandomScalar) << std::endl;
    std::cout << "#### invinvRandomScalar : "
              << *toHexString(_eccCryptoImpl->scalarInvert(invRandomScalar)) << std::endl;

    auto ecPoint = _eccCryptoImpl->hashToCurve(hashResult);
    BOOST_CHECK(_eccCryptoImpl->isValidEcPoint(ecPoint));
    // check (ecPoint^randomScalar)^invRandomScalar == ecPoint
    auto multiPoint = _eccCryptoImpl->ecMultiply(ecPoint, randomScalar);
    for (int i = 0; i < 1000; i++)
    {
        BOOST_CHECK(_eccCryptoImpl->ecMultiply(ecPoint, randomScalar) == multiPoint);
    }
    BOOST_CHECK(_eccCryptoImpl->isValidEcPoint(multiPoint));
    auto invPoint = _eccCryptoImpl->ecMultiply(multiPoint, invRandomScalar);
    for (int i = 0; i < 1000; i++)
    {
        BOOST_CHECK(_eccCryptoImpl->ecMultiply(multiPoint, invRandomScalar) == invPoint);
    }
    BOOST_CHECK(_eccCryptoImpl->isValidEcPoint(invPoint));
    std::cout << "#### invPoint: " << *toHexString(invPoint) << std::endl;
    std::cout << "#### eccPoint : " << *toHexString(ecPoint) << std::endl;
    BOOST_CHECK(invPoint == ecPoint);
    // check the point size
    BOOST_CHECK(invPoint.size() == _eccCryptoImpl->pointSizeInBytes());
    BOOST_CHECK(ecPoint.size() == _eccCryptoImpl->pointSizeInBytes());

    // test ecPointAdd and Sub
    auto addResult = _eccCryptoImpl->ecAdd(invPoint, ecPoint);
    BOOST_CHECK(_eccCryptoImpl->isValidEcPoint(addResult));
    auto subResult = _eccCryptoImpl->ecSub(addResult, invPoint);
    BOOST_CHECK(_eccCryptoImpl->isValidEcPoint(subResult));
    BOOST_CHECK(subResult == ecPoint);


    bytes invalidPoint;
    BOOST_CHECK(_eccCryptoImpl->isValidEcPoint(invalidPoint) == false);

    // invalid point multipy
    ecPoint.resize(16);
    BOOST_CHECK_THROW(_eccCryptoImpl->ecMultiply(ecPoint, randomScalar), EcMultipleException);
    // mul G
    std::string scalar = "abbc1415ad291c4c2d3a6f26a2a6c7dc0da2690418e55e9fe73342b9fed69a03";
    std::string mulGPoint = "5efd75e128f35f24158920bfa597a720883d876698c52d9a02d916e46599fc11";
    if (_ed25519)
    {
        BOOST_CHECK(
            _eccCryptoImpl->mulGenerator(*fromHexString(scalar)) == *fromHexString(mulGPoint));
    }

    // batch for mul G
    for (int i = 0; i < 1000; i++)
    {
        BOOST_CHECK(_eccCryptoImpl->mulGenerator(randomScalar) ==
                    _eccCryptoImpl->mulGenerator(randomScalar));
    }
    // test ECDHCryptoImpl
    auto cryptoBox = std::make_shared<ppc::crypto::CryptoBox>(_hashImpl, _eccCryptoImpl);
    ECDHCrypto::Ptr serverEcdhCrypto = nullptr;
    ECDHCrypto::Ptr clientEcdhCrypto = nullptr;
    auto serverPrivateKey = _eccCryptoImpl->generateRandomScalar();
    auto clientPrivateKey = _eccCryptoImpl->generateRandomScalar();
    if (!_testIppCrypto)
    {
        serverEcdhCrypto = std::make_shared<ECDHCryptoImpl>(serverPrivateKey, cryptoBox);
        clientEcdhCrypto = std::make_shared<ECDHCryptoImpl>(clientPrivateKey, cryptoBox);
    }
    else
    {
#ifdef ENABLE_CRYPTO_MB
        if (ppc::CPU_FEATURES.avx512ifma)
        {
            std::cout << "#### test IppECDHCryptoImpl" << std::endl;
            serverEcdhCrypto = std::make_shared<IppECDHCryptoImpl>(serverPrivateKey, _hashImpl);
            clientEcdhCrypto = std::make_shared<IppECDHCryptoImpl>(clientPrivateKey, _hashImpl);
        }
        else
        {
            std::cout << "#### return directly without test IppECDHCryptoImpl for not support "
                         "avx512ifma instruction"
                      << std::endl;
            return;
        }
#else
        return;
#endif
    }
    // batchGetPublicKey
    auto dataBatch = std::make_shared<ppc::io::DataBatch>();
    dataBatch->setDataSchema(DataSchema::Bytes);
    std::vector<bcos::bytes> inputDatas;
    for (int i = 0; i < 100; i++)
    {
        std::string item = std::to_string(i);
        inputDatas.emplace_back(bcos::bytes(item.begin(), item.end()));
    }
    dataBatch->setData(std::move(inputDatas));

    auto publicKeys = serverEcdhCrypto->batchGetPublicKey(dataBatch);
    auto sharedPublicKeys1 = clientEcdhCrypto->batchGetSharedPublicKey(publicKeys);

    publicKeys = clientEcdhCrypto->batchGetPublicKey(dataBatch);
    auto sharedPublicKeys2 = serverEcdhCrypto->batchGetSharedPublicKey(publicKeys);

    // check the result
    BOOST_CHECK(sharedPublicKeys1.size() == sharedPublicKeys2.size());
    for (uint64_t i = 0; i < sharedPublicKeys1.size(); i++)
    {
        BOOST_CHECK(sharedPublicKeys1.at(i) == sharedPublicKeys2.at(i));
    }
}

BOOST_AUTO_TEST_CASE(testEd25519EccCrypto)
{
    Hash::Ptr hashImpl = std::make_shared<Sha512Hash>();
    testEccCrypto(std::make_shared<Ed25519EccCrypto>(), hashImpl, true, false);
    testEccCrypto(std::make_shared<Ed25519EccCrypto>(), hashImpl, true, true);

    hashImpl = std::make_shared<Sha256Hash>();
    testEccCrypto(std::make_shared<Ed25519EccCrypto>(), hashImpl, true, false);
    testEccCrypto(std::make_shared<Ed25519EccCrypto>(), hashImpl, true, true);

    hashImpl = std::make_shared<SM3Hash>();
    testEccCrypto(std::make_shared<Ed25519EccCrypto>(), hashImpl, true, false);
    testEccCrypto(std::make_shared<Ed25519EccCrypto>(), hashImpl, true, true);
}
BOOST_AUTO_TEST_CASE(testSM2EccCrypto)
{
    Hash::Ptr hashImpl = std::make_shared<Sha512Hash>();
    OpenSSLEccCrypto::Ptr eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::SM2);
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);

    hashImpl = std::make_shared<Sha256Hash>();
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);

    hashImpl = std::make_shared<SM3Hash>();
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);
}

BOOST_AUTO_TEST_CASE(testSecp256K1EccCrypto)
{
    Hash::Ptr hashImpl = std::make_shared<Sha512Hash>();
    OpenSSLEccCrypto::Ptr eccCrypto =
        std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::SECP256K1);
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);

    hashImpl = std::make_shared<Sha256Hash>();
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);

    hashImpl = std::make_shared<SM3Hash>();
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);
}

BOOST_AUTO_TEST_CASE(testP256EccCrypto)
{
    Hash::Ptr hashImpl = std::make_shared<Sha512Hash>();
    OpenSSLEccCrypto::Ptr eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::P256);
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);

    hashImpl = std::make_shared<Sha256Hash>();
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);

    hashImpl = std::make_shared<SM3Hash>();
    testOpenSSLEccCrypto(eccCrypto, hashImpl);
    testEccCrypto(eccCrypto, hashImpl, false, false);
    testEccCrypto(eccCrypto, hashImpl, false, true);
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
