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
 * @file ECDHCryptoImpl.h
 * @author: yujiechen
 * @date 2022-12-29
 */
#pragma once
#include "ppc-framework/crypto/CryptoBox.h"
#include "ppc-framework/crypto/ECDHCrypto.h"
#include <tbb/parallel_for.h>

namespace ppc::crypto
{
class ECDHCryptoImpl : public ECDHCrypto
{
public:
    using Ptr = std::shared_ptr<ECDHCryptoImpl>;
    ECDHCryptoImpl(bcos::bytes const& _privateKey, CryptoBox::Ptr const& _cryptoBox)
      : m_privateKey(_privateKey), m_cryptoBox(_cryptoBox)
    {}

    // calculate the ecdh public-key according to privateKey and input
    std::vector<bcos::bytes> batchGetPublicKey(ppc::io::DataBatch::Ptr const& _input) override
    {
        std::vector<bcos::bytes> result;
        result.resize(_input->size());
        tbb::parallel_for(tbb::blocked_range<size_t>(0U, _input->size()), [&](auto const& range) {
            for (auto i = range.begin(); i < range.end(); i++)
            {
                auto const& data = _input->getBytes(i);
                auto hashData =
                    m_cryptoBox->hashImpl()->hash(bcos::bytesConstRef(data.data(), data.size()));
                auto point = m_cryptoBox->eccCrypto()->hashToCurve(hashData);
                result[i] = m_cryptoBox->eccCrypto()->ecMultiply(point, m_privateKey);
            }
        });
        return result;
    }

    // calculate the ecdh shared-publicKey according to the publicKey and privateKey
    std::vector<bcos::bytes> batchGetSharedPublicKey(
        std::vector<bcos::bytes> const& _publicKeys) override
    {
        std::vector<bcos::bytes> result;
        result.resize(_publicKeys.size());
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0U, _publicKeys.size()), [&](auto const& range) {
                for (auto i = range.begin(); i < range.end(); i++)
                {
                    result[i] =
                        m_cryptoBox->eccCrypto()->ecMultiply(_publicKeys.at(i), m_privateKey);
                }
            });
        return result;
    }

private:
    bcos::bytes m_privateKey;
    CryptoBox::Ptr m_cryptoBox;
};
}  // namespace ppc::crypto