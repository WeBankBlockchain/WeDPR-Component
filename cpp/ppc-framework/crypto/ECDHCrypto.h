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
 * @file ECDHCrypto.h
 * @author: yujiechen
 * @date 2022-12-28
 */
#pragma once
#include "../io/DataBatch.h"
#include "ppc-framework/crypto/CryptoBox.h"
#include <memory>
namespace ppc::crypto
{
class ECDHCrypto
{
public:
    using Ptr = std::shared_ptr<ECDHCrypto>;
    ECDHCrypto() = default;
    virtual ~ECDHCrypto() = default;

    // calculate the ecdh public-key according to privateKey and input
    virtual std::vector<bcos::bytes> batchGetPublicKey(ppc::io::DataBatch::Ptr const& _input) = 0;
    // calculate the ecdh shared-publicKey according to the publicKey and privateKey
    virtual std::vector<bcos::bytes> batchGetSharedPublicKey(
        std::vector<bcos::bytes> const& _publicKey) = 0;
};

class ECDHCryptoFactory
{
public:
    using Ptr = std::shared_ptr<ECDHCryptoFactory>;
    ECDHCryptoFactory() = default;
    virtual ~ECDHCryptoFactory() = default;

    virtual ECDHCrypto::Ptr createECDHCrypto(int _curveType, int _hashType) const = 0;

    virtual CryptoBox::Ptr createCryptoBox(int8_t _curveType, int8_t _hashType) const = 0;
};
}  // namespace ppc::crypto