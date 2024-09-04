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
 * @file BaseOT.h
 * @author: asherli
 * @date 2023-03-13
 */

#pragma once

#include "ppc-framework/Common.h"
#include "ppc-framework/crypto/EccCrypto.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/crypto/SymCrypto.h"

#include "ppc-framework/protocol/Protocol.h"
#include <memory>
#include <utility>
#include <vector>


namespace ppc::crypto
{
struct SenderMessage
{
    std::string taskId;
    // std::vector<bcos::bytes> idIndexList;
    // 私有保存的密钥
    bcos::bytes scalarBlidingA;
    bcos::bytes scalarBlidingB;
    // 发送的数据
    bcos::bytes pointX;
    bcos::bytes pointY;
    bcos::bytes pointZ;
    // 筛选前缀
    bcos::bytes sendObfuscatedHash;
};


struct ReceiverMessage
{
    std::string path;
    std::vector<bcos::bytes> pointWList;
    // 加密的AES key, baseOT交互的结果
    std::vector<bcos::bytes> encryptMessagePair;
    // 加密的AES cipher
    std::vector<bcos::bytes> encryptCipher;
};

class BaseOT : public std::enable_shared_from_this<BaseOT>
{
public:
    using Ptr = std::shared_ptr<BaseOT>;

    BaseOT() = delete;
    ~BaseOT() = default;

    BaseOT(EccCrypto::Ptr _ecc, Hash::Ptr _hash);


    // sender use id_list = [id1,id2,...], generate bliding a, b
    // calculate X = a * G, Y = b * G, c_id = a * b, z_delta = c_id * Gm z_0 = (c_id - id) * G
    // z_0_list = [z_0_id1,z_0_id2,...]
    // obfuscatedOrder披露多少个字符
    SenderMessage senderGenerateCipher(
        const bcos::bytes _choicesMessage, const uint32_t obfuscatedOrder);

    // receiver generate random ri, si
    // calculate wi = si * X + ri * G, zi = z0 + id * G
    // key = si * zi + ri * Y, Ei = key xor mi
    // mi is AES key for Enc(message)
    ReceiverMessage receiverGenerateMessage(const bcos::bytes pointX, const bcos::bytes pointY,
        const std::vector<std::pair<bcos::bytes, bcos::bytes>> messageKeypair,
        const bcos::bytes pointZ);

    // calculate b * G xor Ei = key
    // AES decrypt message
    bcos::bytes finishSender(const bcos::bytes scalarBlidingB,
        const std::vector<bcos::bytes> pointWList,
        const std::vector<bcos::bytes> encryptMessagePair, std::vector<bcos::bytes> encryptCipher);

    std::vector<std::pair<bcos::bytes, bcos::bytes>> prepareDataset(
        bcos::bytes sendObfuscatedHash, std::string datasetPath);


private:
    EccCrypto::Ptr m_ecc;
    Hash::Ptr m_hash;
    bcos::bytes m_iv = bcos::bytes(0);
    ppc::crypto::SymCrypto::Ptr m_acculator;
};

}  // namespace ppc::crypto
