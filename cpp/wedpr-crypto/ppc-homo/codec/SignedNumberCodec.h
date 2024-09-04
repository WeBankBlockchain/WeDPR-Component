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
 * @file SignedNumberCodec.h
 * @author: yujiechen
 * @date 2023-08-09
 */
#pragma once
#include "ppc-framework/Common.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include <memory>

namespace ppc::homo
{
DERIVE_PPC_EXCEPTION(BigNumOutOfRange);
DERIVE_PPC_EXCEPTION(SignedNumberCodecException);
class SignedNumberCodec
{
public:
    using Ptr = std::shared_ptr<SignedNumberCodec>;
    SignedNumberCodec(unsigned _maxBits)
    {
        ppc::crypto::BigNum one(1);
        if (BN_lshift(m_n.bn().get(), one.bn().get(), _maxBits) != 1)
        {
            BOOST_THROW_EXCEPTION(
                SignedNumberCodecException() << bcos::errinfo_comment(
                    "BN_lshift error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
        init();
    }
    // n is the public key
    SignedNumberCodec(ppc::crypto::BigNum const& _n)
    {
        m_n = _n;
        init();
    }

    // _v mod n
    ppc::crypto::BigNum encode(BIGNUM const* v) const
    {
        check(v);
        auto ctx = ppc::crypto::createBNContext();
        ppc::crypto::BigNum result;
        BN_nnmod(result.bn().get(), v, m_n.bn().get(), ctx.get());
        // for negative case check
        check(result.bn().get());
        return result;
    }

    // _v - ( v >= N/3) * _v
    ppc::crypto::BigNum decode(ppc::crypto::BigNum const& _v) const
    {
        check(_v.bn().get());
        if (BN_cmp(_v.bn().get(), m_maxPositive.bn().get()) < 0)
        {
            return _v;
        }
        return _v.sub(m_n.bn().get());
    }
    ppc::crypto::BigNum const& n() const { return m_n; }
    ppc::crypto::BigNum const& maxPositive() const { return m_maxPositive; }
    ppc::crypto::BigNum const& negativeZero() const { return m_negativeZero; }

private:
    void check(BIGNUM const* v) const
    {
        if (BN_cmp(v, m_n.bn().get()) >= 0)
        {
            BOOST_THROW_EXCEPTION(
                SignedNumberCodecException() << bcos::errinfo_comment("Invalid number for over N"));
        }
        if (BN_cmp(v, m_maxPositive.bn().get()) > 0 && BN_cmp(v, m_negativeZero.bn().get()) < 0)
        {
            BOOST_THROW_EXCEPTION(SignedNumberCodecException()
                                  << bcos::errinfo_comment("Invalid number for overflow"));
        }
    }

    void init()
    {
        ppc::crypto::BigNum three(3);
        auto ctx = ppc::crypto::createBNContext();
        // calculate m_maxPositive
        m_n.div(m_maxPositive.bn().get(), NULL, three.bn().get(), ctx.get());
        // calculate m_negativeZero
        ppc::crypto::BigNum two(2);
        m_n.mul(m_negativeZero.bn().get(), two.bn().get(), ctx.get());
        m_negativeZero.div(m_negativeZero.bn().get(), NULL, three.bn().get(), ctx.get());
    }

private:
    ppc::crypto::BigNum m_n;
    ppc::crypto::BigNum m_maxPositive;   // N/3
    ppc::crypto::BigNum m_negativeZero;  // 2*N/3
};
}  // namespace ppc::homo