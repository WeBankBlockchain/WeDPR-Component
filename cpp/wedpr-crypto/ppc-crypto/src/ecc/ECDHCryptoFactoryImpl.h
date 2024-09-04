/**
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
 * @file ECDHCryptoFactoryImpl.h
 * @author: yujiechen
 * @date 2023-1-3
 */
#pragma once
#include "../ecc/EccCryptoFactoryImpl.h"
#include "ECDHCryptoImpl.h"
#include "IppECDHCryptoImpl.h"
#include "ppc-crypto-core/src/hash/HashFactoryImpl.h"
#include "ppc-framework/crypto/CryptoBox.h"
namespace ppc::crypto
{
class ECDHCryptoFactoryImpl : public ECDHCryptoFactory
{
public:
    using Ptr = std::shared_ptr<ECDHCryptoFactoryImpl>;
    ECDHCryptoFactoryImpl(bcos::bytes const& _privateKey)
      : m_privateKey(_privateKey),
        m_hashFactory(std::make_shared<HashFactoryImpl>()),
        m_eccCryptoFactory(std::make_shared<EccCryptoFactoryImpl>())
    {}

    ~ECDHCryptoFactoryImpl() override = default;

    ECDHCrypto::Ptr createECDHCrypto(int _curveType, int _hashType) const override
    {
        auto hashImpl = m_hashFactory->createHashImpl(_hashType);
#ifdef ENABLE_CRYPTO_MB
        if (ppc::CPU_FEATURES.avx512ifma && _curveType == (int)ppc::protocol::ECCCurve::IPP_X25519)
        {
            return std::make_shared<IppECDHCryptoImpl>(m_privateKey, hashImpl);
        }
#endif
        if (_curveType == (int)ppc::protocol::ECCCurve::IPP_X25519)
        {
            BOOST_THROW_EXCEPTION(
                UnsupportedCurveType() << bcos::errinfo_comment(
                    "Not supported IPP_X25519 for missing avx512ifma instruction"));
        }
        auto eccCrypto = m_eccCryptoFactory->createEccCrypto(_curveType, hashImpl);
        auto cryptoBox = std::make_shared<CryptoBox>(hashImpl, eccCrypto);
        return std::make_shared<ECDHCryptoImpl>(m_privateKey, cryptoBox);
    }

    CryptoBox::Ptr createCryptoBox(int8_t _curveType, int8_t _hashType) const override
    {
        auto hashImpl = m_hashFactory->createHashImpl(_hashType);
        auto eccCrypto = m_eccCryptoFactory->createEccCrypto(_curveType, hashImpl);
        return std::make_shared<CryptoBox>(hashImpl, eccCrypto);
    }

private:
    bcos::bytes m_privateKey;
    HashFactory::Ptr m_hashFactory;
    EccCryptoFactory::Ptr m_eccCryptoFactory;
};
}  // namespace ppc::crypto