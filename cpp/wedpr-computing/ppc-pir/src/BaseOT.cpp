/**
 *  Copyright (C) 2021 WeDPR.
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
 * @file BaseOT.cpp
 * @author: asherli
 * @date 2023-03-13
 */

#include "BaseOT.h"
#include "Common.h"
#include "ppc-crypto-core/src/sym-crypto/OpenSSLAES.h"
#include "ppc-framework/crypto/SymCrypto.h"
#include <bcos-utilities/BoostLog.h>
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>


using namespace ppc::crypto;
using namespace ppc::protocol;
using namespace ppc::pir;
using namespace bcos;

BaseOT::BaseOT(EccCrypto::Ptr _ecc, Hash::Ptr _hash)
  : m_ecc(std::move(_ecc)), m_hash(std::move(_hash))
{
    m_acculator = std::make_shared<OpenSSLAES>(OpenSSLAES::AESType::AES128);
}

std::string genRandom(const int len)
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
    {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}


bool isPrefix(bcos::bytes prefixBytes, bcos::bytes targetBytes)
{
    int len1 = prefixBytes.size();
    int len2 = targetBytes.size();

    // If prefix is larger than target, then prefix can't be a prefix anymore.
    if (len1 > len2)
    {
        return false;
    }

    // Check if the first len1 bytes of "targetBytes" are equals to "prefixBytes".
    for (int i = 0; i < len1; i++)
    {
        if (prefixBytes[i] != targetBytes[i])
        {
            return false;
        }
    }

    return true;
}


SenderMessage BaseOT::senderGenerateCipher(
    const bcos::bytes _choicesMessage, const uint32_t obfuscatedOrder)

{
    PIR_LOG(INFO) << LOG_DESC("senderGenerateCipher");

    bcos::bytes scalarBlidingA = m_ecc->generateRandomScalar();
    bcos::bytes scalarBlidingB = m_ecc->generateRandomScalar();
    bcos::bytes pointX = m_ecc->mulGenerator(scalarBlidingA);
    bcos::bytes pointY = m_ecc->mulGenerator(scalarBlidingB);
    bcos::bytes scalarC = m_ecc->scalarMul(scalarBlidingA, scalarBlidingB);
    // PIR_LOG(INFO) << LOG_DESC("scalarBlidingA") << LOG_KV("scalarBlidingA",
    // toHex(scalarBlidingA)); bcos::bytes scalarC_B = m_ecc->scalarSub(scalarC, scalarBlidingB);
    // PIR_LOG(INFO) << LOG_DESC("scalarC_B") << LOG_KV("scalarC_B", toHex(scalarC_B));


    bcos::bytes pointZ = m_ecc->mulGenerator(scalarC);
    // std::vector<bcos::bytes> z0List;
    // std::vector<bcos::bytes> sendObfuscatedHash;
    uint32_t reviewLength = 0;
    if (_choicesMessage.size() < obfuscatedOrder)
    {
        reviewLength = _choicesMessage.size();
    }
    else
    {
        reviewLength = obfuscatedOrder;
    }
    // 取vector的前k个字符
    bcos::bytes sendObfuscatedHash(
        _choicesMessage.begin(), _choicesMessage.begin() + obfuscatedOrder);
    bcos::bytes scalarId = m_ecc->hashToScalar(_choicesMessage);
    PIR_LOG(INFO) << LOG_DESC("scalarId") << LOG_KV("scalarId", toHex(scalarId));

    bcos::bytes scalarCSub = m_ecc->scalarSub(scalarC, scalarId);
    bcos::bytes z0 = m_ecc->mulGenerator(scalarCSub);


    SenderMessage senderMessage;
    senderMessage.pointX = pointX;
    senderMessage.pointY = pointY;
    senderMessage.scalarBlidingA = scalarBlidingA;
    senderMessage.scalarBlidingB = scalarBlidingB;
    senderMessage.pointZ = z0;
    senderMessage.sendObfuscatedHash = sendObfuscatedHash;
    return senderMessage;
}

ReceiverMessage BaseOT::receiverGenerateMessage(const bcos::bytes pointX, const bcos::bytes pointY,
    const std::vector<std::pair<bcos::bytes, bcos::bytes>> messageKeypair, const bcos::bytes pointZ)
{
    PIR_LOG(INFO) << LOG_DESC("receiverGenerateMessage")
                  << LOG_KV("messageKeypair size", messageKeypair.size());
    PIR_LOG(INFO) << LOG_DESC("receiverGenerateMessage") << LOG_KV("pointX", toHex(pointX))
                  << LOG_KV("pointY", toHex(pointY)) << LOG_KV("pointZ", toHex(pointZ));
    // std::vector<bcos::bytes> encryptMessagePair(messageKeypair.size());
    // std::vector<bcos::bytes> pointWList(messageKeypair.size());
    // std::vector<bcos::bytes> encryptCipher(messageKeypair.size());
    std::vector<bcos::bytes> encryptMessagePair;
    std::vector<bcos::bytes> pointWList;
    std::vector<bcos::bytes> encryptCipher;
    for (uint32_t i = 0; i < messageKeypair.size(); ++i)
    {
        // PIR_LOG(TRACE) << LOG_DESC("round") << LOG_KV("i", i);
        bcos::bytes scalarBlidingR = m_ecc->generateRandomScalar();
        bcos::bytes scalarBlidingS = m_ecc->generateRandomScalar();
        bcos::bytes pointWLeft = m_ecc->ecMultiply(pointX, scalarBlidingS);
        bcos::bytes pointWRight = m_ecc->mulGenerator(scalarBlidingR);
        bcos::bytes pointW = m_ecc->ecAdd(pointWLeft, pointWRight);
        bcos::bytes pointKeyRight = m_ecc->ecMultiply(pointY, scalarBlidingR);
        // std::cout << "pointW" << toHex(pointW) << std::endl;
        pointWList.push_back(pointW);
        bcos::bytes scalarId = m_ecc->hashToScalar(messageKeypair[i].first);
        // PIR_LOG(INFO) << LOG_DESC("scalarId") << LOG_KV("scalarId", toHex(scalarId));

        // std::cout << "iter->first" << std::string(messageKeypair[i].first.begin(),
        // messageKeypair[i].first.end()) << std::endl;

        // point是33个字节uint_8
        bcos::bytes pointZRight = m_ecc->mulGenerator(scalarId);
        // PIR_LOG(INFO) << LOG_DESC("pointZRight") << LOG_KV("pointZRight.size()",
        // pointZRight.size());

        // TODO: use random aes key 128bit
        bcos::bytes message = messageKeypair[i].second;
        // std::vector<bcos::bytes> encryptMessageList;
        bcos::bytes defaultPrefix(pir::DEFAULT_PREFIX.begin(), pir::DEFAULT_PREFIX.end());
        bcos::bytes randomKey = m_ecc->generateRandomScalar();


        // 截取defaultPrefix和randomKey的前16个字节拼接成一个新的bcos::bytes类型
        bcos::bytes newBytes;
        bcos::bytes aesKey;
        aesKey.insert(aesKey.end(), randomKey.begin(), randomKey.begin() + 16);
        newBytes.insert(newBytes.end(), defaultPrefix.begin(), defaultPrefix.begin() + 16);
        newBytes.insert(newBytes.end(), randomKey.begin(), randomKey.begin() + 16);
        bcos::bytes encryptedData = m_acculator->encrypt(
            SymCrypto::OperationMode::CBC, bcos::ref(aesKey), bcos::ref(m_iv), bcos::ref(message));
        encryptCipher.push_back(encryptedData);
        // PIR_LOG(INFO) << LOG_DESC("aesKey") << LOG_KV("aesKey", toHex(aesKey));

        bcos::bytes pointZi = m_ecc->ecAdd(pointZ, pointZRight);
        bcos::bytes pointKeyLeft = m_ecc->ecMultiply(pointZi, scalarBlidingS);
        bcos::bytes pointKey = m_ecc->ecAdd(pointKeyLeft, pointKeyRight);
        // PIR_LOG(INFO) << LOG_DESC("pointKey") << LOG_KV("pointKey", toHex(pointKey));
        // PIR_LOG(INFO) << LOG_DESC("message") << LOG_KV("message",
        // std::string(message.begin(),message.end()));

        if (pointKey.size() < newBytes.size())
        {
            BOOST_THROW_EXCEPTION(
                CipherLengthFailException() << errinfo_comment("encrypt ot newBytes fail"));
        }
        bcos::bytes encryptMessage;
        for (uint32_t k = 0; k < newBytes.size(); ++k)
        {
            encryptMessage.push_back(newBytes[k] ^ pointKey[k]);
            // encryptMessage.push_back(message[k]);
        }
        encryptMessagePair.push_back(encryptMessage);
    }
    PIR_LOG(INFO) << LOG_DESC("init result");
    ReceiverMessage result;
    result.pointWList = pointWList;
    result.encryptMessagePair = encryptMessagePair;
    result.encryptCipher = encryptCipher;
    PIR_LOG(INFO) << LOG_DESC("end ReceiverMessage");
    return result;
}

bcos::bytes BaseOT::finishSender(const bcos::bytes scalarBlidingB,
    const std::vector<bcos::bytes> pointWList, const std::vector<bcos::bytes> encryptMessagePair,
    std::vector<bcos::bytes> encryptCipher)
{
    PIR_LOG(INFO) << LOG_DESC("finishSender");
    // std::vector<int> indexList;
    std::vector<bcos::bytes> decryptKeyList;
    if (encryptMessagePair.size() != pointWList.size())
    {
        PIR_LOG(ERROR) << LOG_DESC("check encryptMessagePair and pointWList fail")
                       << LOG_KV("encryptMessagePair.size", encryptMessagePair.size())
                       << LOG_KV("pointWList.size", pointWList.size());
        BOOST_THROW_EXCEPTION(CipherLengthFailException()
                              << errinfo_comment("check sendObfuscatedHash and pointWList fail"));
    }

    for (uint32_t i = 0; i < pointWList.size(); ++i)
    {
        bcos::bytes pointW = pointWList[i];
        PIR_LOG(INFO) << LOG_DESC("BaseOT::finishSender") << LOG_KV("pointW", toHex(pointW));

        bcos::bytes messagePair = encryptMessagePair[i];
        bcos::bytes key = m_ecc->ecMultiply(pointW, scalarBlidingB);
        PIR_LOG(INFO) << LOG_DESC("key") << LOG_KV("key", toHex(key));
        bcos::bytes decryptMessage;
        for (uint32_t k = 0; k < messagePair.size(); ++k)
        {
            decryptMessage.push_back(messagePair[k] ^ key[k]);
            // decryptMessage.push_back(messagePair[i][k]);
        }
        PIR_LOG(INFO) << LOG_DESC("decryptMessage")
                      << LOG_KV("decryptMessage",
                             std::string(decryptMessage.begin(), decryptMessage.end()));
        if (isPrefix(bcos::bytes(DEFAULT_PREFIX.begin(), DEFAULT_PREFIX.end()), decryptMessage))
        {
            // 截取bcos::bytes的16-32个字节
            bcos::bytes aesKey(decryptMessage.begin() + 16, decryptMessage.begin() + 32);
            PIR_LOG(INFO) << LOG_DESC("aesKey") << LOG_KV("aesKey", toHex(aesKey));
            PIR_LOG(INFO) << LOG_DESC("encryptCipher")
                          << LOG_KV("encryptCipher[i]", toHex(encryptCipher[i]));
            bcos::bytes decryptData(m_acculator->blockSize());
            OutputBuffer decryptedBytes{decryptData.data(), decryptData.size()};
            m_acculator->decrypt(&decryptedBytes, SymCrypto::OperationMode::CBC, bcos::ref(aesKey),
                bcos::ref(m_iv), bcos::ref(encryptCipher[i]));
            PIR_LOG(INFO) << LOG_DESC("decryptData")
                          << LOG_KV("decryptData",
                                 std::string(decryptData.begin(), decryptData.end()));

            return decryptData;
        }
    }

    return bcos::bytes();
}


std::vector<std::pair<bcos::bytes, bcos::bytes>> BaseOT::prepareDataset(
    bcos::bytes sendObfuscatedHashPrefix, std::string datasetPath)
{
    PIR_LOG(INFO) << LOG_DESC("prepareDataset")
                  << LOG_KV(
                         "sendObfuscatedHashPrefix", std::string(sendObfuscatedHashPrefix.begin(),
                                                         sendObfuscatedHashPrefix.end()));
    PIR_LOG(INFO) << LOG_DESC("prepareDataset") << LOG_KV("datasetPath", datasetPath);

    std::vector<std::pair<bcos::bytes, bcos::bytes>> messageKeypair =
        pir::readInputsByMmapWithPrefix(datasetPath, sendObfuscatedHashPrefix);

    return messageKeypair;
}
