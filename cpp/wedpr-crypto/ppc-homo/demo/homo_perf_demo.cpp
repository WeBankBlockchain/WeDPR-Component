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
 * @file homo_perf_demo.cpp
 * @author: yujiechen
 * @date 2023-08-09
 */
#include "ppc-framework/libwrapper/BigNum.h"
#include "ppc-homo/codec/FloatingPointCodec.h"
#include "ppc-homo/codec/SignedNumberCodec.h"
#include "ppc-homo/fahe/Fahe.h"
#include "ppc-homo/paillier/FloatingPointPaillier.h"
#include "ppc-homo/paillier/OpenSSLPaillier.h"

using namespace bcos;
using namespace ppc;
using namespace ppc::crypto;
using namespace ppc::homo;

std::pair<double, double> getTPS(int64_t _endT, int64_t _startT, size_t _count)
{
    auto tps = (1000000.0 * (double)_count) / (double)(_endT - _startT);
    auto timeCostUs = (1000000.0) / tps;
    return std::make_pair(tps, timeCostUs);
}

void paillierPerf(int const _keyBits, size_t _loopCount)
{
    auto paillier = std::make_shared<OpenSSLPaillier>();

    std::cout << "----------- paillier perf start -----------" << std::endl;
    std::cout << "\t keyBits: " << _keyBits << " , loopCount: " << _loopCount << std::endl;
    srand(bcos::utcSteadyTime());
    /// generate keyPair
    KeyPair::UniquePtr keyPair;
    auto startT = utcSteadyTimeUs();
    for (size_t i = 0; i < _loopCount; i++)
    {
        keyPair = paillier->generateKeyPair(_keyBits);
    }
    auto pk = (PaillierPublicKey*)(keyPair->pk());
    auto codec = std::make_shared<SignedNumberCodec>(pk->n);
    auto generateKeyPairTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* GenerateKeyPair TPS: " << generateKeyPairTPS.first
              << "\ttimecost per count: " << generateKeyPairTPS.second << " us" << std::endl;

    /// encrypt
    startT = utcSteadyTimeUs();
    bcos::bytes cipherData;
    int64_t value = 0;
    for (size_t i = 0; i < _loopCount; i++)
    {
        value = i % 2 ? rand() : (-rand());
        BigNum v(value);
        cipherData = paillier->encrypt_with_crt(v.bn().get(), (void*)keyPair.get());
    }
    auto encryptTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Encrypt TPS: " << encryptTPS.first
              << "\t\ttimecost per count: " << encryptTPS.second << " us" << std::endl;

    /// decrypt
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < _loopCount; i++)
    {
        auto result = paillier->decrypt(bcos::ref(cipherData), (void*)keyPair.get());
    }
    auto decryptTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Decrypt TPS: " << decryptTPS.first
              << "\t\ttimecost per count: " << decryptTPS.second << " us" << std::endl;

    /// paillier add
    startT = utcSteadyTimeUs();
    bcos::bytes addCipher;
    for (size_t i = 0; i < _loopCount; i++)
    {
        addCipher = paillier->add(bcos::ref(cipherData), bcos::ref(cipherData), keyPair->pk());
    }
    auto addTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Add TPS: " << addTPS.first << "\t\ttimecost per count: " << addTPS.second
              << " us" << std::endl;

    /// paillier sub
    startT = utcSteadyTimeUs();
    bcos::bytes subCipher;
    for (size_t i = 0; i < _loopCount; i++)
    {
        subCipher = paillier->sub(bcos::ref(cipherData), bcos::ref(cipherData), keyPair->pk());
    }
    auto subTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Sub TPS: " << subTPS.first << "\t\ttimecost per count: " << subTPS.second
              << " us" << std::endl;
    /// paillier scalaMul
    startT = utcSteadyTimeUs();
    bcos::bytes scalaMulCipher;
    for (size_t i = 0; i < _loopCount; i++)
    {
        BigNum v(value);
        scalaMulCipher = paillier->scalaMul(v.bn().get(), bcos::ref(cipherData), keyPair->pk());
    }
    auto scalaMulTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* ScalaMul TPS: " << scalaMulTPS.first
              << "\t\ttimecost per count: " << scalaMulTPS.second << " us" << std::endl;
    std::cout << "----------- paillier perf end -----------" << std::endl;
    std::cout << std::endl;
}

void floatingPointPaillierPerf(int const _keyBits, size_t _loopCount, bool _needAlign)
{
    std::cout << "----------- floating-point-paillier perf start, need align: " << _needAlign
              << "-----------" << std::endl;
    std::cout << "\t keyBits: " << _keyBits << " , loopCount: " << _loopCount << std::endl;
    auto paillierImpl = std::make_shared<OpenSSLPaillier>();
    auto fpPaillier = std::make_shared<FloatingPointPaillier>(paillierImpl);
    /// prepare
    // generate keyPair
    auto keyPair = paillierImpl->generateKeyPair(_keyBits);
    auto pk = keyPair->pk();
    std::string s1("1.23432");
    std::string s2("234234.234234324");
    std::string s3("23423.133243");

    float50 m1(s1);
    float50 m2(s2);
    float50 v(s3);
    /// Note: the align operation has the overhead of scalaMul
    if (!_needAlign)
    {
        m1 = float50("10.0");
        m2 = float50("20.0");
        v = float50("1");
    }
    auto codec = std::make_shared<FloatingPointCodec>();
    auto c1Fp = codec->toFloatingPoint(s1);
    auto c2Fp = codec->toFloatingPoint(s2);
    auto vFp = codec->toFloatingPoint(s3);
    // precision to 10
    double epsilon = 0.00000000001;
    /// encrypt
    bcos::bytes c1(FloatingPointPaillier::maxCipherBytesLen(_keyBits));
    OutputBuffer c1Buffer{c1.data(), c1.size()};
    bcos::bytes c2(FloatingPointPaillier::maxCipherBytesLen(_keyBits));
    OutputBuffer c2Buffer{c2.data(), c2.size()};
    auto startT = utcSteadyTimeUs();
    for (size_t i = 0; i < _loopCount; i++)
    {
        fpPaillier->encrypt(&c1Buffer, c1Fp, pk);
    }
    auto encryptTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Encrypt TPS: " << encryptTPS.first
              << "\t\ttimecost per count: " << encryptTPS.second << " us" << std::endl;
    fpPaillier->encrypt(&c2Buffer, c2Fp, pk);
    /// decrypt
    startT = utcSteadyTimeUs();
    FloatingPointNumber result;
    for (size_t i = 0; i < _loopCount; i++)
    {
        result = fpPaillier->decrypt(ref(c1), (void*)keyPair.get());
    }
    auto decryptTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Decrypt TPS: " << decryptTPS.first
              << "\t\ttimecost per count: " << decryptTPS.second << " us" << std::endl;
    // check the result
    assert(fabs(codec->toFloat50(result) - m1) <= epsilon);
    /// add
    bcos::bytes addResult(FloatingPointPaillier::maxCipherBytesLen(_keyBits));
    OutputBuffer addResultBuffer{addResult.data(), addResult.size()};
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < _loopCount; i++)
    {
        fpPaillier->add(&addResultBuffer, ref(c1), ref(c2), pk);
    }
    auto addTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Add TPS: " << addTPS.first << "\t\ttimecost per count: " << addTPS.second
              << " us" << std::endl;

    /// sub
    bcos::bytes subResult(FloatingPointPaillier::maxCipherBytesLen(_keyBits));
    OutputBuffer subResultBuffer{subResult.data(), subResult.size()};
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < _loopCount; i++)
    {
        fpPaillier->sub(&subResultBuffer, ref(c1), ref(c2), pk);
    }
    auto subTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* Sub TPS: " << subTPS.first << "\t\ttimecost per count: " << subTPS.second
              << " us" << std::endl;
    /// scalaMul
    bcos::bytes scalaMulResult(FloatingPointPaillier::maxCipherBytesLen(_keyBits));
    OutputBuffer scalaMulResultBuffer{scalaMulResult.data(), scalaMulResult.size()};
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < _loopCount; i++)
    {
        fpPaillier->scalaMul(&scalaMulResultBuffer, vFp, ref(c1), pk);
    }
    auto scalaMulTPS = getTPS(utcSteadyTimeUs(), startT, _loopCount);
    std::cout << "* ScalaMul TPS: " << scalaMulTPS.first
              << "\t\ttimecost per count: " << scalaMulTPS.second << " us" << std::endl;
    std::cout << "----------- floating-point-paillier perf finished -----------" << std::endl;
}

void fahePerf(int lambda, int alpha, int mmax, size_t loopCount)
{
    std::cout << "----------- fahe perf start -----------" << std::endl;
    std::cout << "* lambda: " << lambda << std::endl;
    std::cout << "* alpha: " << alpha << std::endl;
    std::cout << "* mmax: " << mmax << std::endl;
    std::cout << "* loopCount: " << loopCount << std::endl;
    auto faheImpl = std::make_shared<Fahe>();
    FaheKey::UniquePtr key;
    auto startT = utcSteadyTimeUs();
    for (size_t i = 0; i < loopCount; i++)
    {
        key = faheImpl->generateKey(lambda, mmax, alpha);
    }
    auto genKeyTPS = getTPS(utcSteadyTimeUs(), startT, loopCount);
    std::cout << "* generateKey TPS: " << genKeyTPS.first
              << "\t\ttimecost per count: " << genKeyTPS.second << " us" << std::endl;
    // encrypt
    BigNum m1(123213231 + bcos::utcSteadyTime());
    BigNum c1;
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < loopCount; i++)
    {
        c1 = faheImpl->encrypt(m1.bn().get(), key.get());
    }
    auto encryptTPS = getTPS(utcSteadyTimeUs(), startT, loopCount);
    std::cout << "* encrypt TPS: " << encryptTPS.first
              << "\t\ttimecost per count: " << encryptTPS.second << " us" << std::endl;
    BigNum m2(-(1232231 + bcos::utcSteadyTime()));
    auto c2 = faheImpl->encrypt(m2.bn().get(), key.get());

    // add
    BigNum addResult;
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < loopCount; i++)
    {
        addResult = faheImpl->add(c1.bn().get(), c2.bn().get());
    }
    auto addTPS = getTPS(utcSteadyTimeUs(), startT, loopCount);
    std::cout << "* fahe add TPS: " << addTPS.first << "\t\ttimecost per count: " << addTPS.second
              << " us" << std::endl;
    // decrypt
    BigNum decryptedResult;
    startT = utcSteadyTimeUs();
    for (size_t i = 0; i < loopCount; i++)
    {
        decryptedResult = faheImpl->decrypt(addResult.bn().get(), key.get());
    }
    auto decryptTPS = getTPS(utcSteadyTimeUs(), startT, loopCount);
    std::cout << "* fahe decrypt TPS: " << decryptTPS.first
              << "\t\ttimecost per count: " << decryptTPS.second << " us" << std::endl;
    // check the result
    auto expectedAddResult = m1.add(m2.bn().get());
    assert(expectedAddResult.cmp(decryptedResult.bn().get()) == 0);
    std::cout << "----------- fahe perf end -----------" << std::endl;

    /// check the correctness of fahe
    std::cout << "----------- fahe perf correctness check -----------" << std::endl;
    uint64_t count = 1000000;
    srand(utcSteadyTime());
    for (size_t i = 0; i < count; i++)
    {
        BigNum randM1(rand() + utcSteadyTime());
        BigNum randM2(rand() + utcSteadyTimeUs());
        auto randC1 = faheImpl->encrypt(randM1.bn().get(), key.get());
        auto randC2 = faheImpl->encrypt(randM2.bn().get(), key.get());
        // add
        auto addCipher = faheImpl->add(randC1.bn().get(), randC2.bn().get());
        // decrypt
        auto addResult = faheImpl->decrypt(addCipher.bn().get(), key.get());
        auto expectedAddResult = randM1.add(randM2.bn().get());
        assert(expectedAddResult.cmp(addResult.bn().get()) == 0);
    }
    std::cout << "----------- fahe perf correctness check finished -----------" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << "\n loopCount [required]"
                  << "\n keyBitLength[optional]: default is 2048" << std::endl;
        return -1;
    }
    int keyBits = 2048;
    int loopCount = atol(argv[1]);
    if (argc >= 3)
    {
        keyBits = atol(argv[2]);
    }
    paillierPerf(keyBits, loopCount);
    floatingPointPaillierPerf(keyBits, loopCount, false);
    floatingPointPaillierPerf(keyBits, loopCount, true);
    int lambda = 128;
    int alpha = 33;
    int mmax = 64;
    fahePerf(lambda, alpha, mmax, loopCount);
    return 0;
}