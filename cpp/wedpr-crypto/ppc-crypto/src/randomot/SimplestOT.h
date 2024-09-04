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
 * @file SimplestOT.h
 * @author: shawnhe
 * @date 2022-12-5
 */

#pragma once

#include "ppc-crypto-core/src/tools/BitVector.h"
#include "ppc-framework/Common.h"
#include "ppc-framework/crypto/EccCrypto.h"
#include "ppc-framework/crypto/Hash.h"
#include "ppc-framework/protocol/Protocol.h"
#include <memory>
#include <utility>

namespace ppc::crypto
{
class SimplestOT : public std::enable_shared_from_this<SimplestOT>
{
public:
    using Ptr = std::shared_ptr<SimplestOT>;

    SimplestOT() = delete;
    ~SimplestOT() = default;

    SimplestOT(EccCrypto::Ptr _ecc, Hash::Ptr _hash)
      : m_ecc(std::move(_ecc)), m_hash(std::move(_hash))
    {}

    // receive point A and evaluate multi points B, return Bs bytes and multi scalar bs bytes
    std::pair<bcos::bytesPointer, std::vector<bcos::bytes>> receiverGeneratePointsB(
        BitVector::Ptr _choices, bcos::bytesPointer _pointA);

    // return keys
    std::vector<bcos::bytes> finishReceiver(
        bcos::bytesPointer _pointA, std::vector<bcos::bytes> _bScalars);

    // generate a random point A
    std::pair<bcos::bytes, bcos::bytesPointer> senderGeneratePointA();

    // receive multi points B and generate keys
    std::vector<std::array<bcos::bytes, 2>> finishSender(
        bcos::bytes _aScalar, bcos::bytesPointer _pointA, bcos::bytesPointer _pointsB);

private:
    EccCrypto::Ptr m_ecc;
    Hash::Ptr m_hash;
};

}  // namespace ppc::crypto
