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
 * @file TestHomoPaillier.cpp
 * @author: yujiechen
 * @date 2023-08-15
 */
#include "ppc-crypto-c-sdk/fast_ore.h"
#include "ppc-crypto-c-sdk/floating_point_ihc.h"
#include "ppc-crypto-c-sdk/homo_ihc.h"
#include "ppc-crypto-c-sdk/homo_paillier.h"
#include "ppc-crypto-c-sdk/utils/error.h"
#include "ppc-framework/crypto/Ihc.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include "ppc-homo/paillier/OpenSSLPaillierKeyPair.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <openssl/bn.h>
#include <openssl/ossl_typ.h>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace bcos::test;
using namespace ppc::homo;
using namespace ppc::crypto;
namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(homoPaillierTest, TestPromptFixture)

void checkPrivateKey(PaillierPrivateKey* sk1, PaillierPrivateKey* sk2)
{
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
void checkPublicKey(PaillierPublicKey* pk1, PaillierPublicKey* pk2)
{
    BOOST_CHECK(pk1->keyBits == pk2->keyBits);
    BOOST_CHECK(pk1->n.cmp(pk2->n.bn().get()) == 0);
    BOOST_CHECK(pk1->nSqrt.cmp(pk2->nSqrt.bn().get()) == 0);
    BOOST_CHECK(pk1->h.cmp(pk2->h.bn().get()) == 0);
    BOOST_CHECK(pk1->h_s.cmp(pk2->h_s.bn().get()) == 0);
}
void testPaillierKeyPairImpl(int _keyBits)
{
    auto keyPair = paillier_generate_keypair(_keyBits);
    auto encodedPk = paillier_get_public_key_bytes_from_keyPair(keyPair);
    auto encodedSk = paillier_get_private_key_bytes_from_keypair(keyPair);
    // load the keypair
    InputBuffer sk{(const unsigned char*)encodedSk.data, encodedSk.len};
    InputBuffer pk{(const unsigned char*)encodedPk.data, encodedPk.len};
    auto decodedKeyPair = paillier_load_keypair(&sk, &pk);
    // check the key pair
    auto pk1 = (PaillierPublicKey*)(((OpenSSLPaillierKeyPair*)keyPair)->pk());
    auto pk2 = (PaillierPublicKey*)(((OpenSSLPaillierKeyPair*)decodedKeyPair)->pk());
    checkPublicKey(pk1, pk2);

    auto sk1 = (PaillierPrivateKey*)(((OpenSSLPaillierKeyPair*)keyPair)->sk());
    auto sk2 = (PaillierPrivateKey*)(((OpenSSLPaillierKeyPair*)decodedKeyPair)->sk());
    checkPrivateKey(sk1, sk2);

    // load the public key
    auto decodedPk = paillier_load_public_key(&pk);
    checkPublicKey(pk1, (PaillierPublicKey*)decodedPk);
}
BOOST_AUTO_TEST_CASE(testPaillierKeyPair)
{
    testPaillierKeyPairImpl(2048);
}

bcos::bytes testEncryptDecrypt(BIGNUM* _value, void* keypair, int _keyBits)
{
    auto pk = ((OpenSSLPaillierKeyPair*)keypair)->pk();
    BigNum n;
    paillier_n_from_pk(n.bn().get(), (void*)pk);
    std::cout << "#### n: " << BN_print_fp(stdout, n.bn().get()) << std::endl;

    bcos::bytes cipher(paillier_max_cipher_bytes(_keyBits), 0);
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    // encrypt fast
    bcos::bytes rBytes(paillier_r_bytes_len(_keyBits));
    OutputBuffer rBuffer{rBytes.data(), rBytes.size()};
    paillier_encryt_fast(&cipherBuffer, &rBuffer, _value, keypair);
    if (!last_call_success())
    {
        std::cout << "### paillier_encryt_fast error for: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    std::cout << "#### rBuffer: " << bcos::toHex(rBytes) << ", rBuffer size: " << rBuffer.len
              << std::endl;

    // encrypt
    bcos::bytes cipher2(paillier_max_cipher_bytes(_keyBits), 0);
    OutputBuffer cipherBuffer2{cipher2.data(), cipher2.size()};
    paillier_encryt(&cipherBuffer2, nullptr, _value, pk);
    if (!last_call_success())
    {
        std::cout << "### paillier_encryt error for: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success() == 1);
    // decrypt with keypair
    InputBuffer inputBuffer{cipherBuffer.data, cipherBuffer.len};
    auto decodeV = paillier_decrypt(&inputBuffer, keypair);
    if (!last_call_success())
    {
        std::cout << "#### paillier decrypt error: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success() == 1);
    BOOST_CHECK(BN_cmp(_value, decodeV) == 0);

    InputBuffer inputBuffer2{cipherBuffer2.data, cipherBuffer2.len};
    decodeV = paillier_decrypt(&inputBuffer2, keypair);
    if (!last_call_success())
    {
        std::cout << "#### paillier decrypt error: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    std::cout << "###### expectedValue: " << std::endl;
    BN_print_fp(stdout, _value);
    std::cout << "###### decodeV:" << std::endl;
    BN_print_fp(stdout, decodeV);
    BOOST_CHECK(BN_cmp(_value, decodeV) == 0);
    cipher.resize(cipherBuffer.len);
    return cipher;
}

void testPaillierOpsImpl(
    BigNum const& m1, BigNum const& m2, BigNum const& v, void* keypair, int keyBits)
{
    auto pk = ((OpenSSLPaillierKeyPair*)keypair)->pk();
    auto c1 = testEncryptDecrypt(m1.bn().get(), keypair, keyBits);
    InputBuffer c1Buffer{c1.data(), c1.size()};

    auto c2 = testEncryptDecrypt(m2.bn().get(), keypair, keyBits);
    InputBuffer c2Buffer{c2.data(), c2.size()};

    /// check m1 + m2
    bcos::bytes cipherResult(paillier_max_cipher_bytes(keyBits), 0);
    OutputBuffer resultBuffer{cipherResult.data(), cipherResult.size()};
    paillier_add(&resultBuffer, &c1Buffer, &c2Buffer, pk);
    if (!last_call_success())
    {
        std::cout << "### paillier_add error for: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    // decrypt and check
    InputBuffer encodedBuffer{resultBuffer.data, resultBuffer.len};
    auto decodeV = paillier_decrypt(&encodedBuffer, keypair);
    if (!last_call_success())
    {
        std::cout << "##### paillier_decrypt error: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    auto addResult = m1.add(m2.bn().get());
    std::cout << "###### expectedAddResult: " << std::endl;
    BN_print_fp(stdout, addResult.bn().get());
    std::cout << "###### addResult:" << std::endl;
    BN_print_fp(stdout, decodeV);
    BOOST_CHECK(BN_cmp(addResult.bn().get(), decodeV) == 0);

    /// check m1 - m2
    OutputBuffer subBuffer{cipherResult.data(), cipherResult.size()};
    paillier_sub(&subBuffer, &c1Buffer, &c2Buffer, pk);
    if (!last_call_success())
    {
        std::cout << "### paillier_sub error for: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    // decrypt and check
    InputBuffer subResultBuffer{subBuffer.data, subBuffer.len};
    decodeV = paillier_decrypt(&subResultBuffer, keypair);
    if (!last_call_success())
    {
        std::cout << "#### paillier decrypt error: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    auto subResult = m1.sub(m2.bn().get());
    std::cout << "###### m1:" << std::endl;
    BN_print_fp(stdout, m1.bn().get());
    std::cout << "###### m2:" << std::endl;
    BN_print_fp(stdout, m2.bn().get());

    std::cout << "###### expectedSubResult: " << std::endl;
    BN_print_fp(stdout, subResult.bn().get());
    std::cout << "###### subResult:" << std::endl;
    BN_print_fp(stdout, decodeV);
    BOOST_CHECK(BN_cmp(subResult.bn().get(), decodeV) == 0);

    /// check v * m1
    OutputBuffer mulBuffer{cipherResult.data(), cipherResult.size()};
    paillier_scala_mul(&mulBuffer, v.bn().get(), &c1Buffer, pk);
    if (!last_call_success())
    {
        std::cout << "### paillier_scala_mul error for: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    InputBuffer mulResultBuffer{mulBuffer.data, mulBuffer.len};
    decodeV = paillier_decrypt(&mulResultBuffer, keypair);
    if (!last_call_success())
    {
        std::cout << "#### paillier decrypt error: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    auto ctx = createBNContext();
    BigNum mulResult;
    v.mul(mulResult.bn().get(), m1.bn().get(), ctx.get());
    std::cout << "###### expectedMulResult: " << std::endl;
    BN_print_fp(stdout, mulResult.bn().get());
    std::cout << "###### mulResult:" << std::endl;
    BN_print_fp(stdout, decodeV);
    BOOST_CHECK(BN_cmp(mulResult.bn().get(), decodeV) == 0);
}
// test paillier encrypt-decrypt
BOOST_AUTO_TEST_CASE(testPaillierEncryptDecrypt)
{
    BigNum value(bcos::utcSteadyTime());
    int keyBits = 2048;
    auto keypair = paillier_generate_keypair(keyBits);
    testEncryptDecrypt(value.bn().get(), keypair, keyBits);
}
// test paillier ops
BOOST_AUTO_TEST_CASE(testPaillierOps)
{
    int64_t m1 = 12323423434 + rand();
    int64_t m2 = 2342342344 + rand();
    int64_t v = 21334234234;
    int keyBits = 2048;
    srand(bcos::utcSteadyTime());

    // positive case
    auto keypair = paillier_generate_keypair(keyBits);
    testPaillierOpsImpl(BigNum(m1), BigNum(m2), BigNum(v), keypair, keyBits);

    // negative case
    m1 = -1234324 - rand();
    testPaillierOpsImpl(BigNum(m1), BigNum(m2), BigNum(v), keypair, keyBits);

    v = -34534 - rand();
    testPaillierOpsImpl(BigNum(m1), BigNum(m2), BigNum(v), keypair, keyBits);
}

bcos::bytes testEncryptionDecryption(bcos::bytes const& skBytes, int mode, BigNum const& m1)
{
    // encrypt
    InputBuffer sk{skBytes.data(), skBytes.size()};
    bcos::bytes cipher(ihc_cipher_bytes(mode));
    std::cout << "##### cipher len: " << cipher.size() << std::endl;
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    ihc_encrypt(&cipherBuffer, mode, &sk, m1.bn().get());
    std::cout << "##### cipher1: " << toHex(cipher) << std::endl;
    // decrypt
    InputBuffer cipherInput{cipher.data(), cipher.size()};
    BigNum resulttmp;
    ihc_decrypt(resulttmp.bn().get(), mode, &sk, &cipherInput);
    if (!last_call_success())
    {
        std::cout << "##### testEncryptionDecryption error: " << get_last_error_msg() << std::endl;
    }
    BOOST_CHECK(last_call_success());
    // check
    std::cout << "#### m1: " << std::endl;
    BN_print_fp(stdout, m1.bn().get());
    std::cout << "#### resulttmp: " << std::endl;
    BN_print_fp(stdout, resulttmp.bn().get());
    BOOST_CHECK(resulttmp.cmp(m1.bn().get()) == 0);
    return cipher;
}
void testIhcImpl(int mode, BigNum const& m1, BigNum const& m2, BigNum const& v)
{
    // generate sk
    bcos::bytes skBytes(ihc_key_bytes(mode));
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    ihc_generate_key(&skBuffer, mode);
    InputBuffer sk{skBytes.data(), skBytes.size()};
    // c1
    auto c1 = testEncryptionDecryption(skBytes, mode, m1);
    // c2
    auto c2 = testEncryptionDecryption(skBytes, mode, m2);
    // add test: m1 + m2
    std::cout << "##### testIhcImpl: add case" << std::endl;
    InputBuffer cipher1Input{c1.data(), c1.size()};
    InputBuffer cipher2Input{c2.data(), c2.size()};
    bcos::bytes cipherSum(ihc_cipher_bytes(mode));
    OutputBuffer cipherBufferSum{cipherSum.data(), cipherSum.size()};
    ihc_add(&cipherBufferSum, mode, &cipher1Input, &cipher2Input);
    std::cout << "##### cipherSum: " << toHex(cipherSum) << std::endl;
    // decrypt
    InputBuffer cipherSumBuffer{cipherSum.data(), cipherSum.size()};
    BigNum addResult;
    ihc_decrypt(addResult.bn().get(), mode, &sk, &cipherSumBuffer);
    // check
    BigNum expected = m1.add(m2.bn().get());
    BOOST_CHECK(expected.cmp(addResult.bn().get()) == 0);
    std::cout << "##### testIhcImpl: add case finish" << std::endl;
}

bcos::bytes testIhcFloatingEncryptionDecryption(
    int mode, bcos::bytes const& skBytes, FloatingPointNumber const& m1)

{
    InputBuffer sk{skBytes.data(), skBytes.size()};
    // encryption
    bcos::bytes cipher(ihc_floating_cipher_bytes(mode));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    ihc_floating_encrypt(&cipherBuffer, mode, &sk, m1.value.bn().get(), m1.exponent);
    std::cout << "cipher: " << toHex(cipher) << std::endl;
    InputBuffer cipherInput{cipher.data(), cipher.size()};

    // decrypt
    BigNum decryptedResult;
    int16_t decryptedExponent = 0;

    ihc_floating_decrypt(decryptedResult.bn().get(), &decryptedExponent, mode, &sk, &cipherInput);
    BOOST_CHECK(m1.exponent == decryptedExponent);
    BOOST_CHECK(m1.value.cmp(decryptedResult.bn().get()) == 0);
    return cipher;
}

void testIhcFloatingImpl(
    int mode, std::string const& s1, std::string const& s2, std::string const& s3)
{
    std::cout << "testIhcFloatingImpl" << std::endl;
    // 生成密钥
    bcos::bytes skBytes(ihc_key_bytes(mode));
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    ihc_generate_key(&skBuffer, mode);
    InputBuffer sk{skBytes.data(), skBytes.size()};

    auto codec = std::make_shared<FloatingPointCodec>();
    auto m1 = codec->toFloatingPoint(s1);
    auto m2 = codec->toFloatingPoint(s2);
    auto m3 = codec->toFloatingPoint(s3);
    float50 f1(s1);
    float50 f2(s2);
    float50 f3(s3);

    auto c1 = testIhcFloatingEncryptionDecryption(mode, skBytes, m1);
    auto c2 = testIhcFloatingEncryptionDecryption(mode, skBytes, m2);
    auto c3 = testIhcFloatingEncryptionDecryption(mode, skBytes, m3);
    // add test
    std::cout << "#### testIhcFloatingImpl: add test" << std::endl;
    bcos::bytes cipherSum(ihc_floating_cipher_bytes(mode));
    OutputBuffer cipherBufferSum{cipherSum.data(), cipherSum.size()};
    InputBuffer c1Buffer{c1.data(), c1.size()};
    InputBuffer c2Buffer{c2.data(), c2.size()};
    ihc_floating_add(&cipherBufferSum, mode, &c1Buffer, &c2Buffer);
    InputBuffer cipherSumInput{cipherSum.data(), cipherSum.size()};
    FloatingPointNumber addResult;
    ihc_floating_decrypt(
        addResult.value.bn().get(), &addResult.exponent, mode, &sk, &cipherSumInput);
    std::cout << std::fixed << std::setprecision(15);
    std::cout << "#### f1 + f2: " << (f1 + f2) << std::endl;
    std::cout << "#### addResult: " << codec->toFloat50(addResult) << std::endl;
    float50 epsilon("0.0000000000001");
    BOOST_CHECK(fabs(codec->toFloat50(addResult) - f1 - f2) <= epsilon);
    std::cout << "#### testIhcFloatingImpl: add test finished" << std::endl;

    // sub test
    std::cout << "#### testIhcFloatingImpl: sub test" << std::endl;
    bcos::bytes cipherSub(ihc_floating_cipher_bytes(mode));
    OutputBuffer cipherSubBuffer{cipherSub.data(), cipherSub.size()};
    ihc_floating_sub(&cipherSubBuffer, mode, &c1Buffer, &c2Buffer);
    InputBuffer cipherSubInput{cipherSub.data(), cipherSub.size()};
    FloatingPointNumber subResult;
    ihc_floating_decrypt(
        subResult.value.bn().get(), &subResult.exponent, mode, &sk, &cipherSubInput);
    std::cout << std::fixed << std::setprecision(15);
    std::cout << "#### f1 - f2: " << (f1 - f2) << std::endl;
    std::cout << "#### subResult: " << codec->toFloat50(subResult) << std::endl;
    BOOST_CHECK(fabs(codec->toFloat50(subResult) - (f1 - f2)) <= epsilon);
    std::cout << "#### testIhcFloatingImpl: sub test finished" << std::endl;

    // scalaMul test
    std::cout << "#### testIhcFloatingImpl: scalaMul test" << std::endl;
    bcos::bytes cipherMul(ihc_floating_cipher_bytes(mode));
    OutputBuffer cipherMulBuffer{cipherMul.data(), cipherMul.size()};
    ihc_floating_scalaMul(&cipherMulBuffer, mode, m3.value.bn().get(), m3.exponent, &c1Buffer);
    InputBuffer cipherMulInput{cipherMul.data(), cipherMul.size()};
    FloatingPointNumber mulResult;
    ihc_floating_decrypt(
        mulResult.value.bn().get(), &mulResult.exponent, mode, &sk, &cipherMulInput);
    BOOST_CHECK(fabs(codec->toFloat50(mulResult) - (f1 * f3)) <= epsilon);
    std::cout << "#### testIhcFloatingImpl: scalaMul test finished" << std::endl;
}

void testIhcItermImpl(int _mode)
{
    // generate sk
    bcos::bytes skBytes(ihc_key_bytes(_mode));
    OutputBuffer skBuffer{skBytes.data(), skBytes.size()};
    ihc_generate_key(&skBuffer, _mode);
    std::cout << "##### skBuffer: " << toHex(skBytes) << std::endl;
    // encrypt
    InputBuffer sk{skBytes.data(), skBytes.size()};
    BigNum m1(10000);
    bcos::bytes cipherSum(ihc_cipher_bytes(_mode));
    OutputBuffer cipherBufferSum{cipherSum.data(), cipherSum.size()};
    BigNum expected(10000);
    ihc_encrypt(&cipherBufferSum, _mode, &sk, m1.bn().get());
    for (int i = 0; i < 100; i++)
    {
        bcos::bytes cipher(ihc_cipher_bytes(_mode));
        OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
        ihc_encrypt(&cipherBuffer, _mode, &sk, m1.bn().get());
        InputBuffer cipherInput{cipher.data(), cipher.size()};
        InputBuffer cipherSumInput{cipherSum.data(), cipherSum.size()};
        ihc_add(&cipherBufferSum, _mode, &cipherSumInput, &cipherInput);
        if (!last_call_success())
        {
            std::cout << "#### ihc add error: " << get_last_error_msg() << std::endl;
        }
        BOOST_CHECK(last_call_success());
        BigNum result;
        ihc_decrypt(result.bn().get(), _mode, &sk, &cipherSumInput);
        expected = expected.add(m1.bn().get());
        BOOST_CHECK(result.cmp(expected.bn().get()) == 0);
    }
}

BOOST_AUTO_TEST_CASE(testIhcOps)
{
    int64_t m1 = 123213231;
    int64_t m2 = 123213231;
    int64_t v = 23;
    int keyBits = 64;
    testIhcImpl((int)Ihc::IhcMode::IHC_128, BigNum(m1), BigNum(m2), BigNum(v));
    // negative case
    m1 = -1234324 - rand();
    testIhcImpl((int)Ihc::IhcMode::IHC_128, BigNum(m1), BigNum(m2), BigNum(v));
    v = -34534 - rand();
    testIhcImpl((int)Ihc::IhcMode::IHC_128, BigNum(m1), BigNum(m2), BigNum(v));

    testIhcItermImpl((int)Ihc::IhcMode::IHC_128);

    std::string s1("-23423423.24534534");
    std::string s2("0.0000234688");
    std::string s3("-1233.002348");
    testIhcFloatingImpl((int)Ihc::IhcMode::IHC_128, s1, s2, s3);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
