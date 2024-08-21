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
 * @file crypto_bench.cpp
 * @desc: bench for io
 * @author: yujiechen
 * @date 2022-12-1
 */
#include "ppc-crypto-core/src/hash/BLAKE2bHash.h"
#include "ppc-crypto-core/src/hash/MD5Hash.h"
#include "ppc-crypto-core/src/hash/SM3Hash.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto-core/src/hash/Sha512Hash.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLAES.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLSM4.h"
#include "ppc-crypto/src/ecc/ECDHCryptoImpl.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-crypto/src/ecc/IppECDHCryptoImpl.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-crypto/src/oprf/RA2018Oprf.h"
#include "ppc-framework/io/DataBatch.h"

using namespace bcos;
using namespace ppc::io;
using namespace ppc::protocol;
using namespace ppc::crypto;

const std::string HASH_CMD = "hash";
const std::string ECC_CMD = "ecc";
const std::string ENCRYPT_CMD = "enc";
const std::string OPRF_CMD = "oprf";
const std::string ECDH_CMD = "ecdh";


void Usage(std::string const& _appName)
{
    std::cout << _appName << " [" << HASH_CMD << "/" << ECC_CMD << "/" << ENCRYPT_CMD << "/"
              << OPRF_CMD << "/" << ECDH_CMD << "] count" << std::endl;
}

double getTPS(int64_t _endT, int64_t _startT, size_t _count)
{
    return (1000.0 * (double)_count) / (double)(_endT - _startT);
}

void hashPerf(
    Hash::Ptr _hash, std::string const& _hashName, std::string const& _inputData, size_t _count)
{
    std::cout << std::endl;
    std::cout << "----------- " << _hashName << " perf start -----------" << std::endl;
    auto startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        _hash->hash(bytesConstRef((byte const*)_inputData.c_str(), _inputData.size()));
    }
    std::cout << "input data size: " << (double)_inputData.size() / 1000.0
              << "KB, loops: " << _count << ", timeCost: " << utcSteadyTime() - startT << " ms"
              << std::endl;
    std::cout << "TPS of " << _hashName << ": "
              << getTPS(utcSteadyTime(), startT, _count) * (double)_inputData.size() / 1000.0
              << " KB/s" << std::endl;
    std::cout << "----------- " << _hashName << " perf end -----------" << std::endl;
    std::cout << std::endl;
}

void hashPerf(size_t _count)
{
    std::string inputData = "abcdwer234q4@#2424wdf";
    std::string deltaData = inputData;
    for (int i = 0; i < 50; i++)
    {
        inputData += deltaData;
    }
    // MD5Hash perf
    Hash::Ptr hashImpl = std::make_shared<MD5Hash>();
    hashPerf(hashImpl, "MD5Hash", inputData, _count);
    // Sha256Hash perf
    hashImpl = std::make_shared<Sha256Hash>();
    hashPerf(hashImpl, "Sha256Hash", inputData, _count);
    // Sha512Hash perf
    hashImpl = std::make_shared<Sha512Hash>();
    hashPerf(hashImpl, "Sha512Hash", inputData, _count);
    // SM3Hash perf
    hashImpl = std::make_shared<SM3Hash>();
    hashPerf(hashImpl, "SM3Hash", inputData, _count);
    // BLAKE2bHash
    hashImpl = std::make_shared<BLAKE2bHash>();
    hashPerf(hashImpl, "BLAKE2bHash", inputData, _count);
}

void eccCryptoPerf(EccCrypto::Ptr _eccCrypto, std::string const& _eccName,
    bcos::bytes const& _hashData, size_t _count)
{
    std::cout << std::endl;
    std::cout << "----------- " << _eccName << " perf start -----------" << std::endl;
    // generateRandomScalar
    auto startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        _eccCrypto->generateRandomScalar();
    }
    std::cout << "TPS of " << _eccName
              << ": generateRandomScalar: " << getTPS(utcSteadyTime(), startT, _count)
              << ", timecost:" << (utcSteadyTime() - startT) << " ms" << std::endl;
    // hashToCurve
    startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        _eccCrypto->hashToCurve(_hashData);
    }
    std::cout << "TPS of " << _eccName
              << ": hashToCurve: " << getTPS(utcSteadyTime(), startT, _count)
              << ", timecost:" << (utcSteadyTime() - startT) << " ms" << std::endl;
    auto point = _eccCrypto->hashToCurve(_hashData);
    auto scalar = _eccCrypto->generateRandomScalar();
    // ecMultiply
    startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        _eccCrypto->ecMultiply(point, scalar);
    }
    std::cout << "TPS of " << _eccName
              << ": ecMultiply: " << getTPS(utcSteadyTime(), startT, _count)
              << ", timecost:" << (utcSteadyTime() - startT) << " ms" << std::endl;
    // scalarInvert
    startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        _eccCrypto->scalarInvert(scalar);
    }
    std::cout << "TPS of " << _eccName
              << ": scalarInvert: " << getTPS(utcSteadyTime(), startT, _count)
              << ", timecost:" << (utcSteadyTime() - startT) << " ms" << std::endl;
    // mulGenerator
    startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        _eccCrypto->mulGenerator(scalar);
    }
    std::cout << "TPS of " << _eccName
              << ": mulGenerator: " << getTPS(utcSteadyTime(), startT, _count)
              << ", timecost:" << (utcSteadyTime() - startT) << " ms" << std::endl;
    std::cout << "----------- " << _eccName << " perf end -----------" << std::endl;
    std::cout << std::endl;
}

void eccCryptoPerf(size_t _count)
{
    auto hashImpl = std::make_shared<Sha256Hash>();
    std::string inputStr = "abcdwer234q4@#2424wdf";
    bytes inputData(inputStr.begin(), inputStr.end());
    auto hashData = hashImpl->hash(ref(inputData));
    // ed25519 perf
    EccCrypto::Ptr eccCrypto = std::make_shared<Ed25519EccCrypto>();
    eccCryptoPerf(eccCrypto, "Ed25519EccCrypto", hashData, _count);
    // sm2 perf
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::SM2);
    eccCryptoPerf(eccCrypto, "OpenSSLEccCrypto-SM2", hashData, _count);
    // prime256v1 perf
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::P256);
    eccCryptoPerf(eccCrypto, "OpenSSLEccCrypto-P256", hashData, _count);
    // secp256k1 perf
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::SECP256K1);
    eccCryptoPerf(eccCrypto, "OpenSSLEccCrypto-SECP256K1", hashData, _count);
}


void encryptPerf(SymCrypto::Ptr _encryptor, OpenSSLAES::OperationMode _mode,
    std::string const& _inputData, const std::string& _encryptorName, size_t _count)
{
    std::string keyString = "abcdefgwerelkewrwerw";
    bcos::bytes key(keyString.begin(), keyString.end());
    std::string ivData = "12334324";
    bcos::bytes iv(ivData.begin(), ivData.end());

    std::cout << std::endl;
    std::cout << "----------- " << _encryptorName << " perf test start -----------" << std::endl;
    // encrypt
    bcos::bytes encryptedData;
    auto startT = utcSteadyTime();
    for (size_t i = 0; i < _count; i++)
    {
        encryptedData = _encryptor->encrypt(_mode, ref(key), ref(iv),
            bytesConstRef((byte const*)_inputData.c_str(), _inputData.size()));
    }
    std::cout << "PlainData size:" << (double)_inputData.size() / 1000.0 << " KB, loops: " << _count
              << ", timeCost: " << utcSteadyTime() - startT << " ms" << std::endl;
    std::cout << "TPS of " << _encryptorName << " encrypt:"
              << (getTPS(utcSteadyTime(), startT, _count) * (double)(_inputData.size())) / 1000.0
              << "KB/s" << std::endl;
    std::cout << std::endl;
    // decrypt
    startT = utcSteadyTime();
    bytes decryptedData;
    for (size_t i = 0; i < _count; i++)
    {
        decryptedData = _encryptor->decrypt(_mode, ref(key), ref(iv), ref(encryptedData));
    }
    std::cout << "CiperData size:" << (double)encryptedData.size() / 1000.0
              << " KB, loops: " << _count << ", timeCost:" << utcSteadyTime() - startT << " ms"
              << std::endl;
    std::cout << "TPS of " << _encryptorName << " decrypt:"
              << (getTPS(utcSteadyTime(), startT, _count) * (double)_inputData.size()) / 1000.0
              << "KB/s" << std::endl;
    bytes plainBytes(_inputData.begin(), _inputData.end());
    assert(plainBytes == decryptedData);

    std::cout << "----------- " << _encryptorName << " perf test end -----------" << std::endl;
    std::cout << std::endl;
}

void encryptPerf(size_t _count)
{
    std::string inputData = "w3rwerk2-304swlerkjewlrjoiur4kslfjsd,fmnsdlfjlwerlwerjw;erwe;rewrew";
    std::string deltaData = inputData;
    for (int i = 0; i < 100; i++)
    {
        inputData += deltaData;
    }

    // AES128-CBC
    SymCrypto::Ptr encryptor = nullptr;
    encryptor = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128);
    encryptPerf(encryptor, SymCrypto::OperationMode::CBC, inputData, "AES128-CBC", _count);

    // AES192-CBC
    encryptor = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES192);
    encryptPerf(encryptor, SymCrypto::OperationMode::CBC, inputData, "AES192-CBC", _count);
    // AES256-CBC
    encryptor = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES256);
    encryptPerf(encryptor, SymCrypto::OperationMode::CBC, inputData, "AES256-CBC", _count);
    // SM4-CBC
    encryptor = std::make_shared<OpenSSLSM4>();
    encryptPerf(encryptor, SymCrypto::OperationMode::CBC, inputData, "SM4-CBC", _count);
}


void RA2018OprfCryptoPerf(
    RA2018Oprf::Ptr _oprf, std::vector<bcos::bytes> const& _inputData, std::string const& _testCase)
{
    std::cout << std::endl;
    std::cout << "----------- " << _testCase << " perf start -----------" << std::endl;
    //// FullEvaluate
    auto tmpData = _inputData;
    auto dataBatch = std::make_shared<DataBatch>();
    dataBatch->setData(std::move(tmpData));

    auto startT = utcSteadyTime();
    std::vector<bcos::bytes> fullEvaluateResult;
    _oprf->fullEvaluate(dataBatch, fullEvaluateResult);
    std::cout << "fullEvaluate: timecost of " << _testCase << " : " << (utcSteadyTime() - startT)
              << " ms" << std::endl;

    /// blind
    startT = utcSteadyTime();
    std::vector<bcos::bytes> blindResult;
    auto privateKey = _oprf->generatePrivateKey();
    _oprf->blind(dataBatch, privateKey, blindResult);
    std::cout << "Blind: timecost of " << _testCase << " : " << (utcSteadyTime() - startT) << " ms"
              << std::endl;

    /// evaluate
    startT = utcSteadyTime();
    std::vector<bcos::bytes> evaluateData;
    _oprf->evaluate(blindResult, evaluateData);
    std::cout << "Evaluate: timecost of " << _testCase << " : " << (utcSteadyTime() - startT)
              << " ms" << std::endl;

    /// finalize
    startT = utcSteadyTime();
    std::vector<bcos::bytes> finalizeResult;
    auto invPrivateKey = _oprf->inv(privateKey);
    _oprf->finalize(evaluateData, invPrivateKey, finalizeResult);
    std::cout << "Finalize: timecost of " << _testCase << " : " << (utcSteadyTime() - startT)
              << " ms" << std::endl;
    std::cout << "----------- " << _testCase << " perf end -----------" << std::endl;
    std::cout << std::endl;
}

void oprfCryptoPerf(size_t _count)
{
    // fake the input data
    std::vector<bcos::bytes> data;
    for (size_t i = 0; i < _count; i++)
    {
        auto inputStr = std::to_string(i);
        data.emplace_back(bcos::bytes(inputStr.begin(), inputStr.end()));
    }

    EccCrypto::Ptr eccCrypto = std::make_shared<Ed25519EccCrypto>();
    // generate the private-key
    auto privateKey = eccCrypto->generateRandomScalar();
    auto hashImpl = std::make_shared<Sha256Hash>();

    /// ed25519 ra2018 oprf
    auto oprf = std::make_shared<RA2018Oprf>(privateKey, eccCrypto, hashImpl);
    RA2018OprfCryptoPerf(oprf, data, "RA2018-OPRF-ED25519");
    // P256 ra2018 oprf
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::P256);
    privateKey = eccCrypto->generateRandomScalar();
    oprf = std::make_shared<RA2018Oprf>(privateKey, eccCrypto, hashImpl);
    RA2018OprfCryptoPerf(oprf, data, "RA2018-OPRF-P256");

    /// sm2 ra2018 oprf
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::SM2);
    privateKey = eccCrypto->generateRandomScalar();
    oprf = std::make_shared<RA2018Oprf>(privateKey, eccCrypto, hashImpl);
    RA2018OprfCryptoPerf(oprf, data, "RA2018-OPRF-SM2");
}

void ecdhCryptoPerf(ECDHCrypto::Ptr _ecdhCrypto, std::vector<bcos::bytes> const& _inputData,
    std::string const& _testCase)
{
    try
    {
        std::cout << std::endl;
        std::cout << "----------- " << _testCase << " perf start -----------" << std::endl;
        //// batchGetPublicKey
        auto tmpData = _inputData;
        auto dataBatch = std::make_shared<DataBatch>();
        dataBatch->setData(std::move(tmpData));

        auto startT = utcSteadyTime();
        auto publicKeyResult = _ecdhCrypto->batchGetPublicKey(dataBatch);
        std::cout << "batchGetPublicKey: timecost of " << _testCase << " : "
                  << (utcSteadyTime() - startT) << " ms" << std::endl;

        /// batchGetSharedPublicKey
        startT = utcSteadyTime();
        auto sharedKeys = _ecdhCrypto->batchGetSharedPublicKey(publicKeyResult);
        std::cout << "batchGetSharedPublicKey: timecost of " << _testCase << " : "
                  << (utcSteadyTime() - startT) << " ms" << std::endl;
        std::cout << "----------- " << _testCase << " perf end -----------" << std::endl;
    }
    catch (std::exception const& e)
    {
        std::cout << "ecdhCryptoPerf exception, error: " << boost::diagnostic_information(e)
                  << std::endl;
    }
    std::cout << std::endl;
}

void ecdhCryptoPerf(size_t _count)
{
    // fake the input data
    std::vector<bcos::bytes> data;
    for (size_t i = 0; i < _count; i++)
    {
        auto inputStr = std::to_string(i);
        data.emplace_back(bcos::bytes(inputStr.begin(), inputStr.end()));
    }

    EccCrypto::Ptr eccCrypto = std::make_shared<Ed25519EccCrypto>();
    // generate the private-key
    auto privateKey = eccCrypto->generateRandomScalar();
    auto hashImpl = std::make_shared<Sha256Hash>();
    auto cryptoBox = std::make_shared<CryptoBox>(hashImpl, eccCrypto);

    /// ed25519 ecdh-crypto
    auto ecdhCrypto = std::make_shared<ECDHCryptoImpl>(privateKey, cryptoBox);
    ecdhCryptoPerf(ecdhCrypto, data, "ECDH-CRYPTO-ED25519");
    // P256 ecdh-crypto
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::P256);
    privateKey = eccCrypto->generateRandomScalar();
    cryptoBox = std::make_shared<CryptoBox>(hashImpl, eccCrypto);
    ecdhCrypto = std::make_shared<ECDHCryptoImpl>(privateKey, cryptoBox);
    ecdhCryptoPerf(ecdhCrypto, data, "ECDH-CRYPTO-P256");

    /// sm2 ecdh-crypto
    eccCrypto = std::make_shared<OpenSSLEccCrypto>(hashImpl, ECCCurve::SM2);
    privateKey = eccCrypto->generateRandomScalar();
    cryptoBox = std::make_shared<CryptoBox>(hashImpl, eccCrypto);
    ecdhCrypto = std::make_shared<ECDHCryptoImpl>(privateKey, cryptoBox);
    ecdhCryptoPerf(ecdhCrypto, data, "ECDH-CRYPTO-SM2");

// ipp x25519 crypto
#ifdef ENABLE_CRYPTO_MB
    if (ppc::CPU_FEATURES.avx512ifma)
    {
        auto ippEdchCrypto = std::make_shared<IppECDHCryptoImpl>(privateKey, hashImpl);
        ecdhCryptoPerf(ippEdchCrypto, data, "IPP-ECDH-CRYPTO-x25519");
    }
    else
    {
        std::cout << "Ignore IPP-ECDH-CRYPTO-x25519 test for crypto_mb not enabled!" << std::endl;
    }
#else
    std::cout << "Ignore IPP-ECDH-CRYPTO-x25519 test for crypto_mb not enabled!" << std::endl;
#endif
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        Usage(argv[0]);
        return -1;
    }
    auto cmd = argv[1];
    size_t count = atoi(argv[2]);
    if (HASH_CMD == cmd)
    {
        hashPerf(count);
    }
    else if (ENCRYPT_CMD == cmd)
    {
        encryptPerf(count);
    }
    else if (ECC_CMD == cmd)
    {
        eccCryptoPerf(count);
    }
    else if (OPRF_CMD == cmd)
    {
        oprfCryptoPerf(count);
    }
    else if (ECDH_CMD == cmd)
    {
        ecdhCryptoPerf(count);
    }
    else
    {
        std::cout << "Invalid subcommand \"" << cmd << "\"" << std::endl;
        Usage(argv[0]);
    }
    return 0;
}
