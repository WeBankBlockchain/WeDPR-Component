/*
 *  Copyright (C) 2023 WeDPR.
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
 * @file TestPaillier.cpp
 * @author: yujiechen
 * @date 2023-08-08
 */
#include "ppc-homo/codec/SignedNumberCodec.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace ppc::homo;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(paillierTest, TestPromptFixture)

void testPaillierEncryptDecrypt(bcos::bytes& _cipherData, OpenSSLPaillier::Ptr const& _impl,
    SignedNumberCodec::Ptr const& _codec, KeyPair::UniquePtr const& _keyPair, int64_t const _m,
    uint64_t const _perfCount)
{
    (void)_codec;
    ////// test encrypt and decrypt, return the cipher data
    // encrypt
    BigNum m(_m);

    auto startT = bcos::utcSteadyTimeUs();
    for (uint64_t i = 0; i < _perfCount; i++)
    {
        _cipherData = _impl->encrypt_with_crt(m.bn().get(), (void*)_keyPair.get());
    }
    std::cout << "#### paillier encrypt perf:" << ((bcos::utcSteadyTimeUs() - startT) / _perfCount)
              << " us" << std::endl;

    /// case not enough resultBuffer;
    bcos::bytes resultBytes(2);
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    BOOST_CHECK_THROW(
        _impl->encrypt_with_crt(&resultBuffer, nullptr, m.bn().get(), (void*)_keyPair.get()),
        std::exception);

    // decrypt
    startT = utcSteadyTimeUs();
    BigNum decryptedV;
    for (uint64_t i = 0; i < _perfCount; i++)
    {
        decryptedV = _impl->decrypt(bcos::ref(_cipherData), (void*)_keyPair.get());
    }
    std::cout << "#### paillier decrypt perf:" << ((bcos::utcSteadyTimeUs() - startT) / _perfCount)
              << " us" << std::endl;
    std::cout << std::endl;

    BN_print_fp(stdout, decryptedV.bn().get());
    BigNum mBN(_m);
    BOOST_CHECK(mBN.cmp(decryptedV.bn().get()) == 0);
}

void testPaillierHomoProperty(OpenSSLPaillier::Ptr const& _impl,
    SignedNumberCodec::Ptr const& _codec, KeyPair::UniquePtr const& _keyPair, int64_t _value,
    bcos::bytes& _c1, bcos::bytes& _c2, int64_t const _m1, int64_t _m2, uint64_t _perfCount)
{
    std::cout << "### c1 size:" << _c1.size() << std::endl;
    std::cout << "### c2 size:" << _c2.size() << std::endl;
    BigNum m1(_m1);
    BigNum m2(_m2);
    // ==== test add =====
    // _c1 + _c2
    bcos::bytes sumCipher;
    auto startT = bcos::utcSteadyTimeUs();
    for (uint64_t i = 0; i < _perfCount; i++)
    {
        sumCipher = _impl->add(bcos::ref(_c1), bcos::ref(_c2), _keyPair->pk());
    }
    std::cout << "#### paillier add perf:" << ((bcos::utcSteadyTimeUs() - startT) / _perfCount)
              << " us" << std::endl;

    /// case not enough resultBuffer;
    bcos::bytes resultBytes(10);
    OutputBuffer resultBuffer{resultBytes.data(), resultBytes.size()};
    BOOST_CHECK_THROW(
        _impl->add(&resultBuffer, bcos::ref(_c1), bcos::ref(_c2), _keyPair->pk()), std::exception);

    // decrypt and check the result
    auto result = _impl->decrypt(bcos::ref(sumCipher), (void*)_keyPair.get());
    auto expectedBN = m1.add(m2.bn().get());
    BOOST_CHECK(expectedBN.cmp(result.bn().get()) == 0);

    // ==== test sub =====
    // _c1 - _c2
    startT = bcos::utcSteadyTimeUs();
    bcos::bytes subCipher;
    for (uint64_t i = 0; i < _perfCount; i++)
    {
        subCipher = _impl->sub(bcos::ref(_c1), bcos::ref(_c2), _keyPair->pk());
    }
    std::cout << "#### paillier sub perf:" << ((bcos::utcSteadyTimeUs() - startT) / _perfCount)
              << " us" << std::endl;
    /// case not enough resultBuffer;
    BOOST_CHECK_THROW(
        _impl->sub(&resultBuffer, bcos::ref(_c1), bcos::ref(_c2), _keyPair->pk()), std::exception);

    result = _impl->decrypt(bcos::ref(subCipher), (void*)_keyPair.get());
    auto expectedSubBN = m1.sub(m2.bn().get());
    BOOST_CHECK(expectedSubBN.cmp(result.bn().get()) == 0);

    // ==== test mul =====
    // _value * _c1
    bcos::bytes mulCipher;
    startT = bcos::utcSteadyTimeUs();
    ppc::crypto::BigNum v(_value);
    for (uint64_t i = 0; i < _perfCount; i++)
    {
        mulCipher = _impl->scalaMul(v.bn().get(), bcos::ref(_c1), _keyPair->pk());
    }
    std::cout << "#### paillier scalMul perf:" << ((bcos::utcSteadyTimeUs() - startT) / _perfCount)
              << " us" << std::endl;
    /// case not enough resultBuffer;
    BOOST_CHECK_THROW(_impl->scalaMul(&resultBuffer, v.bn().get(), bcos::ref(_c1), _keyPair->pk()),
        std::exception);

    // decrypt and check the result
    auto resultBn = _codec->decode(_impl->decrypt(bcos::ref(mulCipher), (void*)_keyPair.get()));
    BigNum valueBn(_value);
    BigNum valueM1(_m1);
    BigNum expectedBn;
    auto ctx = createBNContext();
    valueBn.mul(expectedBn.bn().get(), valueM1.bn().get(), ctx.get());
    std::cout << "## scalaMul resultBn: ";
    BN_print_fp(stdout, resultBn.bn().get());
    std::cout << std::endl;
    std::cout << "## scalaMul expectedResult: ";
    BN_print_fp(stdout, expectedBn.bn().get());
    std::cout << std::endl;
    BOOST_CHECK(expectedBn.cmp(resultBn.bn().get()) == 0);
}

void checkKeyPair(KeyPair::UniquePtr&& _keyPair1, OpenSSLPaillierKeyPair::UniquePtr&& _keyPair2)
{
    auto pk1 = (PaillierPublicKey*)_keyPair1->pk();
    auto pk2 = (PaillierPublicKey*)_keyPair2->pk();
    BOOST_CHECK(pk1->keyBits == pk2->keyBits);
    BOOST_CHECK(pk1->n.cmp(pk2->n.bn().get()) == 0);
    BOOST_CHECK(pk1->nSqrt.cmp(pk2->nSqrt.bn().get()) == 0);
    BOOST_CHECK(pk1->h.cmp(pk2->h.bn().get()) == 0);

    BOOST_CHECK(pk1->h_s.cmp(pk2->h_s.bn().get()) == 0);

    auto sk1 = (PaillierPrivateKey*)_keyPair1->sk();
    auto sk2 = (PaillierPrivateKey*)_keyPair2->sk();
    BOOST_CHECK(sk1->keyBits == sk2->keyBits);
    BOOST_CHECK(sk1->lambda.cmp(sk2->lambda.bn().get()) == 0);
    BOOST_CHECK(sk1->p.cmp(sk2->p.bn().get()) == 0);
    BOOST_CHECK(sk1->q.cmp(sk2->q.bn().get()) == 0);
    BOOST_CHECK(sk1->pSqrt.cmp(sk2->pSqrt.bn().get()) == 0);
    BOOST_CHECK(sk1->qSqrt.cmp(sk2->qSqrt.bn().get()) == 0);
    BOOST_CHECK(sk1->qSqrtInverse.cmp(sk2->qSqrtInverse.bn().get()) == 0);
    BOOST_CHECK(sk1->pOrderSqrt.cmp(sk2->pOrderSqrt.bn().get()) == 0);
    BOOST_CHECK(sk1->qOrderSqrt.cmp(sk2->qOrderSqrt.bn().get()) == 0);
}

void testOverflow(SignedNumberCodec::Ptr codec, OpenSSLPaillier::Ptr paillierImpl,
    ppc::crypto::KeyPair::UniquePtr&& keyPair)
{
    // overflow case
    auto v = codec->maxPositive();
    BigNum tmp(1);
    tmp.add(tmp.bn().get(), v.bn().get());
    BOOST_CHECK_THROW(paillierImpl->encrypt_with_crt(tmp.bn().get(), (void*)keyPair.get()),
        SignedNumberCodecException);

    // add overflow
    BigNum two(2);
    auto m1 = tmp.sub(two.bn().get());
    auto c1 = paillierImpl->encrypt_with_crt(m1.bn().get(), (void*)keyPair.get());
    BigNum m2(1000);
    auto c2 = paillierImpl->encrypt_with_crt(m2.bn().get(), (void*)keyPair.get());
    auto pk = (PaillierPublicKey*)(keyPair->pk());
    auto cipher = paillierImpl->add(bcos::ref(c1), bcos::ref(c2), (void*)pk);
    // decrypt failed for overflow
    BOOST_CHECK_THROW(
        paillierImpl->decrypt(bcos::ref(cipher), (void*)keyPair.get()), SignedNumberCodecException);
}

BOOST_AUTO_TEST_CASE(testPaillier)
{
    auto paillierImpl = std::make_shared<OpenSSLPaillier>();
    // generate keyPair with keyLength bits
    unsigned keyLength = 3072;
    auto startT = bcos::utcSteadyTime();
    auto keyPair = paillierImpl->generateKeyPair(keyLength);
    std::cout << "### generate keypair timecost:" << (bcos::utcSteadyTime() - startT) << " ms"
              << std::endl;
    auto pk = (PaillierPublicKey*)(keyPair->pk());

    auto codec = std::make_shared<SignedNumberCodec>(pk->n);

    /// === normal test case
    // check encrypt/decrypt
    int64_t perfCount = 2;
    bcos::bytes cipherData1;
    srand(bcos::utcSteadyTime());
    int64_t m1 = 12323423434 + rand();
    testPaillierEncryptDecrypt(cipherData1, paillierImpl, codec, keyPair, m1, perfCount);

    bcos::bytes cipherData2;
    int64_t m2 = 2342342344 + rand();
    testPaillierEncryptDecrypt(cipherData2, paillierImpl, codec, keyPair, m2, perfCount);

    // check homo-property
    int value = rand();
    testPaillierHomoProperty(
        paillierImpl, codec, keyPair, value, cipherData1, cipherData2, m1, m2, perfCount);

    ////// negative case
    m1 = -1234324 - rand();
    testPaillierEncryptDecrypt(cipherData1, paillierImpl, codec, keyPair, m1, 1);
    m2 = 23434 + rand();
    testPaillierEncryptDecrypt(cipherData2, paillierImpl, codec, keyPair, m2, 1);
    testPaillierHomoProperty(
        paillierImpl, codec, keyPair, value, cipherData1, cipherData2, m1, m2, perfCount);

    value = -34534 - rand();
    testPaillierHomoProperty(
        paillierImpl, codec, keyPair, value, cipherData1, cipherData2, m1, m2, perfCount);

    testOverflow(codec, paillierImpl, std::move(keyPair));
}

// test paillier keyPair
BOOST_AUTO_TEST_CASE(testPaillierKeyPair)
{
    int keyLength = 2048;
    auto paillierImpl = std::make_shared<OpenSSLPaillier>();
    auto keyPair = paillierImpl->generateKeyPair(keyLength);

    auto skData = keyPair->serializeSK();
    auto pkData = keyPair->serializePK();

    // create new keyPair according to skData and pkData
    auto keyPair2 = std::make_unique<OpenSSLPaillierKeyPair>(
        skData.data(), skData.size(), pkData.data(), pkData.size());
    // check serialize again
    auto skData2 = keyPair2->serializeSK();
    auto pkData2 = keyPair2->serializePK();
    BOOST_CHECK(skData == skData2);
    BOOST_CHECK(pkData == pkData2);
    checkKeyPair(std::move(keyPair), std::move(keyPair2));
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test