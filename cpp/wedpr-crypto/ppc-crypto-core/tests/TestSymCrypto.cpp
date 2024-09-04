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
 * @file TestSymCrypto.cpp
 * @author: shawnhe
 * @date 2022-11-29
 */
#pragma execution_character_set("utf-8")
#include "ppc-crypto-core/src/sym-crypto/OpenSSL3DES.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLAES.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLSM4.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::protocol;
using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(SymCryptoTest, TestPromptFixture)

void runSymCrypto(SymCrypto::OperationMode _mode, SymCrypto::Ptr _symCryptoEnc,
    std::string const& _input, bool _useSpecifiedKey = false)
{
    u128 key = 1234567, iv = 7654321;
    bcos::bytes bkey;
    bcos::toBigEndian(key, bkey);
    bcos::bytes biv;
    bcos::toBigEndian(iv, biv);
    if (!_useSpecifiedKey)
    {
        bkey = _symCryptoEnc->generateKey(_mode);
    }
    auto cipher = _symCryptoEnc->encrypt(_mode, bcos::ref(bkey), bcos::ref(biv),
        bcos::bytesConstRef((bcos::byte const*)_input.data(), _input.size()));
    auto plain = _symCryptoEnc->decrypt(_mode, bcos::ref(bkey), bcos::ref(biv), bcos::ref(cipher));
    std::cout << "### _input: " << _input << std::endl;
    std::cout << "### plain: " << std::string(plain.begin(), plain.end()) << std::endl;
    BOOST_CHECK(_input == std::string(plain.begin(), plain.end()));
}

void runSymCrypto(
    SymCrypto::OperationMode _mode, SymCrypto::Ptr _symCryptoEnc, bool _useSpecifiedKey)
{
    runSymCrypto(_mode, _symCryptoEnc, "a1", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4e", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4e5", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4e5f6", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4e5f6g7", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4e5f6g7h", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "a1b2c3d4e5f6g7h8", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "謇", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "鬱", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "齉", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "躞", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "&", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "^", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "……", _useSpecifiedKey);
    runSymCrypto(_mode, _symCryptoEnc, "@", _useSpecifiedKey);

    runSymCrypto(_mode, _symCryptoEnc,
        "test symmetric encryption 12345678 12345678 12345678 12345678 12345678 12345678"
        "12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 "
        "12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 "
        "12345678",
        _useSpecifiedKey);

    runSymCrypto(_mode, _symCryptoEnc,
        "测试特殊符号*&（……%￥ 123abc 12345678 12345678 12345678 12345678 12345678 12345678"
        "12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 "
        "12345678 12345678 12345678 12345678 12345678 12345678 12345678 #$%#&$#$*^&(#%$@#()_()",
        _useSpecifiedKey);

    runSymCrypto(_mode, _symCryptoEnc,
        "测试生僻字謇鬱齉躞*&（……%￥ 123abc 12345678 12345678 12345678 12345678 12345678 12345678"
        "12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 12345678 "
        "12345678 12345678 12345678 12345678 12345678 12345678 12345678 #$%#&$#$*^&(#%$@#()_()",
        _useSpecifiedKey);
}

void runSymCryptoEnc(SymCrypto::OperationMode _mode, SymCrypto::Ptr _symCryptoEnc,
    const std::string& _input, const std::string& _expected)
{
    std::string key = "0123456789abcdef0123456789abcdef";
    std::string iv = "0123456789abcdef";

    bcos::bytes bkey(key.begin(), key.end());
    bcos::bytes biv(iv.begin(), iv.end());

    auto cipher = _symCryptoEnc->encrypt(
        _mode, ref(bkey), ref(biv), bcos::bytesConstRef((bcos::byte*)_input.data(), _input.size()));
    std::cout << "cipher: " << *toHexString(cipher) << std::endl;
    BOOST_CHECK(_expected == *toHexString(cipher));
}

void testAESEncDec(bool _useSpecifiedKey)
{
    // test ecb
    std::cout << "====== testSM4EncDesc =====" << std::endl;
    auto sm4Enc = std::make_shared<OpenSSLSM4>();
    std::cout << "====== testSM4EncDesc ECB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::ECB, sm4Enc, _useSpecifiedKey);
    // test cbc
    std::cout << "====== testSM4EncDesc CBC=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CBC, sm4Enc, _useSpecifiedKey);
    std::cout << "====== testSM4EncDesc CFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CFB, sm4Enc, _useSpecifiedKey);
    std::cout << "====== testSM4EncDesc OFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::OFB, sm4Enc, _useSpecifiedKey);
    std::cout << "====== testSM4EncDesc CTR=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CTR, sm4Enc, _useSpecifiedKey);
    std::cout << "====== testSM4EncDesc finished =====" << std::endl;

    std::cout << "====== testAES128EncDec =====" << std::endl;
    auto aes128Enc = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128);
    std::cout << "====== testAES128EncDec ECB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::ECB, aes128Enc, _useSpecifiedKey);
    std::cout << "====== testAES128EncDec CBC=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CBC, aes128Enc, _useSpecifiedKey);
    std::cout << "====== testAES128EncDec CFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CFB, aes128Enc, _useSpecifiedKey);
    std::cout << "====== testAES128EncDec OFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::OFB, aes128Enc, _useSpecifiedKey);
    std::cout << "====== testAES128EncDec CTR=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CTR, aes128Enc, _useSpecifiedKey);
    std::cout << "====== testAES128EncDec finished =====" << std::endl;

    std::cout << "====== testAES192EncDec =====" << std::endl;
    auto aes192Enc = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES192);
    std::cout << "====== testAES192EncDec ECB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::ECB, aes192Enc, _useSpecifiedKey);
    std::cout << "====== testAES192EncDec CBC=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CBC, aes192Enc, _useSpecifiedKey);
    std::cout << "====== testAES192EncDec CFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CFB, aes192Enc, _useSpecifiedKey);
    std::cout << "====== testAES192EncDec OFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::OFB, aes192Enc, _useSpecifiedKey);
    std::cout << "====== testAES192EncDec CTR=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CTR, aes192Enc, _useSpecifiedKey);
    std::cout << "====== testAES192EncDec finished=====" << std::endl;

    std::cout << "====== testAES256EncDec =====" << std::endl;
    auto aes256Enc = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES256);
    std::cout << "====== aes256Enc ECB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::ECB, aes256Enc, _useSpecifiedKey);
    std::cout << "====== aes256Enc CBC=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CBC, aes256Enc, _useSpecifiedKey);
    std::cout << "====== aes256Enc CFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CFB, aes256Enc, _useSpecifiedKey);
    std::cout << "====== aes256Enc OFB=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::OFB, aes256Enc, _useSpecifiedKey);
    std::cout << "====== aes256Enc CTR=====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CTR, aes256Enc, _useSpecifiedKey);
    std::cout << "====== testAES256EncDec finished=====" << std::endl;

    auto desEnc = std::make_shared<OpenSSL3DES>();
    std::cout << "====== test3DESEncDec =====" << std::endl;
    std::cout << "====== test3DESEncDec: ECB =====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::ECB, desEnc, _useSpecifiedKey);
    std::cout << "====== test3DESEncDec: CBC =====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CBC, desEnc, _useSpecifiedKey);
    std::cout << "====== test3DESEncDec: CFB =====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::CFB, desEnc, _useSpecifiedKey);
    std::cout << "====== test3DESEncDec: OFB =====" << std::endl;
    runSymCrypto(SymCrypto::OperationMode::OFB, desEnc, _useSpecifiedKey);
    std::cout << "====== test3DESEncDec finished=====" << std::endl;
}

BOOST_AUTO_TEST_CASE(testSymCryptoPKCS7)
{
    std::cout << "===== testAESEncDec ======" << std::endl;
    testAESEncDec(true);
    std::cout << "===== testAESEncDec finished ======" << std::endl;

    std::cout << "===== testAESEncDec and generateKey ======" << std::endl;
    testAESEncDec(false);
    std::cout << "===== testAESEncDec and generateKey finished ======" << std::endl;
}

BOOST_AUTO_TEST_CASE(testSymCryptoEncPKCS7)
{
    std::cout << "====== testSymCryptoEncPKCS7 =====" << std::endl;
    std::string input = "Hello, World, Hello Tomorrow.";

    // test ecb
    auto sm4Enc = std::make_shared<OpenSSLSM4>();
    runSymCryptoEnc(SymCrypto::OperationMode::ECB, sm4Enc, input,
        "b654e713da62ef2899233b2542c234461ef8aa28a7faef3a089bbffd4e7f708b");
    runSymCryptoEnc(SymCrypto::OperationMode::CBC, sm4Enc, input,
        "5ac1fe687a1227eb8a03d593b2a9a6d4c2d24d6a5bb08c3f4a9427d917fc6643");
    runSymCryptoEnc(SymCrypto::OperationMode::OFB, sm4Enc, input,
        "5b42e00fd81c7e6b7c6eb7dfcad5d3e39645db8c7da36770ebf35f7033");
    runSymCryptoEnc(SymCrypto::OperationMode::CTR, sm4Enc, input,
        "5b42e00fd81c7e6b7c6eb7dfcad5d3e39288a332b76f342309f73b9940");
    runSymCryptoEnc(SymCrypto::OperationMode::CFB, sm4Enc, input,
        "5b42e00fd81c7e6b7c6eb7dfcad5d3e34b9b7ccde57e48d4309e1aa33a");


    auto aes128Enc = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128);
    runSymCryptoEnc(SymCrypto::OperationMode::ECB, aes128Enc, input,
        "2f491097abdda469351be5c2a86c56fc2804e80bd08ac540cf5c30d11658c7f2");
    runSymCryptoEnc(SymCrypto::OperationMode::CBC, aes128Enc, input,
        "edd475c62aae977b0cf5a3e6f94c95dbd19955211fd2cbd3df5517a6888c42a7");
    runSymCryptoEnc(SymCrypto::OperationMode::OFB, aes128Enc, input,
        "3a1712e471f0dd566fd5740c5529fd000819bd65e9f99987da63e1160e");
    runSymCryptoEnc(SymCrypto::OperationMode::CTR, aes128Enc, input,
        "3a1712e471f0dd566fd5740c5529fd009da1a4302925f3fe44d7212132");
    runSymCryptoEnc(SymCrypto::OperationMode::CFB, aes128Enc, input,
        "3a1712e471f0dd566fd5740c5529fd00bfdaf41af70405dfeb98b01730");


    auto aes192Enc = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES192);
    runSymCryptoEnc(SymCrypto::OperationMode::ECB, aes192Enc, input,
        "926dcc7c086c08663e4c98628c2cc1c55a1c991fc93c37ca1cf7dbc6c660ee60");
    runSymCryptoEnc(SymCrypto::OperationMode::CBC, aes192Enc, input,
        "34d5d3dbd184f0f123c96e98880fe877a3dc3364206c062e1d1427c1570244f7");

    runSymCryptoEnc(SymCrypto::OperationMode::CFB, aes192Enc, input,
        "92096245a9be5272896d356998ca309251817976f6521bd51c4ac87ec7");
    runSymCryptoEnc(SymCrypto::OperationMode::OFB, aes192Enc, input,
        "92096245a9be5272896d356998ca30929553d60e30f9d84ce15bbdf080");
    runSymCryptoEnc(SymCrypto::OperationMode::CTR, aes192Enc, input,
        "92096245a9be5272896d356998ca3092b92fe5fcc9efeaf1e48a3c56b0");


    auto aes256Enc = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES256);
    runSymCryptoEnc(SymCrypto::OperationMode::ECB, aes256Enc, input,
        "11f963d26ca05c4a7551e810e692f122f4d57e90d658c7e1ff88dc1432ec170e");
    runSymCryptoEnc(SymCrypto::OperationMode::CBC, aes256Enc, input,
        "8937a782e67fd1d56e6e4b8d614e4afe3b1c9084e60d304ffb6546576e397428");
    runSymCryptoEnc(SymCrypto::OperationMode::CFB, aes256Enc, input,
        "b059f60cb320fbcf4eed15b2f9fb5e5050e7b2936fe1ff1be3387e5c35");
    runSymCryptoEnc(SymCrypto::OperationMode::OFB, aes256Enc, input,
        "b059f60cb320fbcf4eed15b2f9fb5e501790fa5fbe89d8b1a16de8e49e");
    runSymCryptoEnc(SymCrypto::OperationMode::CTR, aes256Enc, input,
        "b059f60cb320fbcf4eed15b2f9fb5e50d9f3075d1b0d4f080e5a633deb");

    auto desEnc = std::make_shared<OpenSSL3DES>();
    runSymCryptoEnc(SymCrypto::OperationMode::ECB, desEnc, input,
        "0bb66fcb35528a9aae4240b9235cd5b403d4e7648bef133d8f145e5475462c26");
    runSymCryptoEnc(SymCrypto::OperationMode::CBC, desEnc, input,
        "a8f91cfb2ab2f98f5f8cb92545a2b3112df69bd94c64e883a9ec84e832cb273d");
    runSymCryptoEnc(SymCrypto::OperationMode::CFB, desEnc, input,
        "04c5b63ce04ad91512c7616c0845f64896a1ffdeb94bbc114b3d23d78b");
    runSymCryptoEnc(SymCrypto::OperationMode::OFB, desEnc, input,
        "04c5b63ce04ad915d7cc3432e00d338268178b71b2ee5ca31433e57916");
    std::cout << "====== testSymCryptoEncPKCS7 finished =====" << std::endl;
}

BOOST_AUTO_TEST_CASE(testSm4)
{
    auto sm4Enc = std::make_shared<OpenSSLSM4>();
    bcos::bytes bkey = sm4Enc->generateKey(SymCrypto::OperationMode::CBC);
    bcos::bytes biv;
    u128 iv = 7654321;
    bcos::toBigEndian(iv, biv);

    bcos::bytes inputs(1024 * 10);

    int count = 10000;
    std::vector<bcos::bytes> ciphers(count);

    auto start = bcos::utcTimeUs();
    for(int i = 0; i < count; i++)
    {
        ciphers[i] = sm4Enc->encrypt(SymCrypto::OperationMode::CBC, bcos::ref(bkey), bcos::ref(biv),
            bcos::bytesConstRef((bcos::byte const*)inputs.data(), inputs.size()));
    }
    std::cout << "CBC, ENC Costs: " << bcos::utcTimeUs() - start<< "us" << std::endl;

    start = bcos::utcTimeUs();
    for(int i = 0; i < count; i++)
    {
        sm4Enc->decrypt(SymCrypto::OperationMode::CBC, bcos::ref(bkey), bcos::ref(biv),
            bcos::bytesConstRef((bcos::byte const*)ciphers[i].data(), ciphers[i].size()));
    }
    std::cout << "CBC, DEC Costs: " << bcos::utcTimeUs() - start<< "us" << std::endl;

    start = bcos::utcTimeUs();
    for(int i = 0; i < count; i++)
    {
        ciphers[i] = sm4Enc->encrypt(SymCrypto::OperationMode::CTR, bcos::ref(bkey), bcos::ref(biv),
            bcos::bytesConstRef((bcos::byte const*)inputs.data(), inputs.size()));
    }
    std::cout << "CTR,  ENC Costs: " << bcos::utcTimeUs() - start<< "us" << std::endl;

    start = bcos::utcTimeUs();
    for(int i = 0; i < count; i++)
    {
        sm4Enc->decrypt(SymCrypto::OperationMode::CTR, bcos::ref(bkey), bcos::ref(biv),
            bcos::bytesConstRef((bcos::byte const*)ciphers[i].data(), ciphers[i].size()));
    }
    std::cout << "CTR, DEC Costs: " << bcos::utcTimeUs() - start<< "us" << std::endl;

}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
