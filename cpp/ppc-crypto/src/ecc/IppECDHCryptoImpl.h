/*
 *  Copyright (C) 2023 WeDPR.
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
 * @file IppECDHCryptoImpl.h
 * @author: yujiechen
 * @date 2023-1-3
 */
#ifdef ENABLE_CRYPTO_MB
#pragma once
#include "../Common.h"
#include "ppc-framework/crypto/ECDHCrypto.h"
#include "ppc-framework/crypto/Hash.h"
#include <crypto_mb/x25519.h>
#include <tbb/parallel_for.h>

namespace ppc::crypto
{
class IppECDHCryptoImpl : public ECDHCrypto
{
public:
    IppECDHCryptoImpl(bcos::bytes const& _privateKey, Hash::Ptr const& _hashImpl)
      : m_privateKeyData(_privateKey), m_hashImpl(_hashImpl)
    {
        std::fill(m_privateKey.begin(), m_privateKey.end(),
            static_cast<const int8u*>(&m_privateKeyData[0]));
    }
    ~IppECDHCryptoImpl() override = default;

    // calculate the ecdh public-key according to privateKey and input
    std::vector<bcos::bytes> batchGetPublicKey(ppc::io::DataBatch::Ptr const& _input) override
    {
        // calculate hash for the input
        std::vector<bcos::bytes> hashResult;
        hashResult.resize(_input->size());
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, _input->size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto const& data = _input->get<bcos::bytes>(i);
                hashResult[i] = m_hashImpl->hash(bcos::bytesConstRef(data.data(), data.size()));
            }
        });
        return batchGetSharedPublicKeyImpl(hashResult);
    }

    // calculate the ecdh shared-publicKey according to the publicKey and privateKey
    std::vector<bcos::bytes> batchGetSharedPublicKey(
        std::vector<bcos::bytes> const& _publicKey) override
    {
        return batchGetSharedPublicKeyImpl(_publicKey);
    }

protected:
    virtual std::vector<bcos::bytes> batchGetSharedPublicKeyImpl(
        std::vector<bcos::bytes> const& _inputPoints)
    {
        std::vector<bcos::bytes> result;
        result.resize(_inputPoints.size());
        uint64_t dataBatchSize =
            (_inputPoints.size() + CRYPTO_MB_BATCH_SIZE - 1) / CRYPTO_MB_BATCH_SIZE;
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, dataBatchSize), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                std::array<const int8u*, CRYPTO_MB_BATCH_SIZE> pk;
                std::array<int8u*, CRYPTO_MB_BATCH_SIZE> sharedKey;
                auto startOffset = i * CRYPTO_MB_BATCH_SIZE;
                uint64_t k = 0;
                int8u junkBuffer[CRYPTO_MB_BATCH_SIZE][X25519_ELEMENT_SIZE];  // Junk buffer
                for (uint64_t j = startOffset; j < startOffset + 8; j++)
                {
                    if (j < result.size())
                    {
                        result[j].resize(X25519_ELEMENT_SIZE);
                        pk[k] = static_cast<const int8u*>(_inputPoints[j].data());
                        sharedKey[k] = static_cast<int8u*>(result[j].data());
                    }
                    else
                    {
                        pk[k] = static_cast<const int8u*>(_inputPoints[startOffset].data());
                        sharedKey[k] = (int8u*)junkBuffer[k];
                    }
                    k++;
                }
                // call mbx_x25519_mb8
                auto status = mbx_x25519_mb8(sharedKey.data(), m_privateKey.data(), pk.data());
                if (status != 0)
                {
                    BOOST_THROW_EXCEPTION(
                        X25519GetSharedKeyError() << bcos::errinfo_comment(
                            "mbx_x25519_mb8 error, status: " + std::to_string(status)));
                }
            }
        });
        return result;
    }

private:
    // enforce the life-time of the privateKey
    bcos::bytes m_privateKeyData;
    // store the address of the privateKey
    std::array<const int8u*, 8> m_privateKey;
    Hash::Ptr m_hashImpl;

    // 32 bytes
    constexpr static int X25519_ELEMENT_SIZE = 32;
    // 8 elements per crypto_mb calls
    constexpr static int CRYPTO_MB_BATCH_SIZE = 8;
};
}  // namespace ppc::crypto
#endif