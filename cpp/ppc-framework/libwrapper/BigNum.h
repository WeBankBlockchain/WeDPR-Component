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
 * @file BigNum.h
 * @author: yujiechen
 * @date 2022-12-5
 */
#pragma once
#include "Buffer.h"
#include "openssl/bn.h"
#include "openssl/err.h"
#include "ppc-framework/Common.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <memory>

namespace ppc::crypto
{
DERIVE_PPC_EXCEPTION(ConvertBytesToBigNumError);
DERIVE_PPC_EXCEPTION(CreateBNContextError);
DERIVE_PPC_EXCEPTION(BigNumSubError);
DERIVE_PPC_EXCEPTION(BigNumAddError);
DERIVE_PPC_EXCEPTION(BigNumMultiError);
DERIVE_PPC_EXCEPTION(GeneratePrimeError);
DERIVE_PPC_EXCEPTION(BigNumGCDError);
DERIVE_PPC_EXCEPTION(BigNumDivError);
DERIVE_PPC_EXCEPTION(BigNumModExpError);
DERIVE_PPC_EXCEPTION(ConvertIntToBigNumError);
DERIVE_PPC_EXCEPTION(BigNumRandError);
DERIVE_PPC_EXCEPTION(BigNumSetWordError);
DERIVE_PPC_EXCEPTION(BigNumGetBytesLenError);
DERIVE_PPC_EXCEPTION(NotEnoughBufferSize);

struct BNCtxDeleter
{
public:
    void operator()(BN_CTX* _bnctx) { BN_CTX_free(_bnctx); }
};
using BNCtxPtr = std::unique_ptr<BN_CTX, BNCtxDeleter>;

struct BNDeleter
{
    void operator()(BIGNUM* bn) { BN_clear_free(bn); }
};
using BNPtr = std::shared_ptr<BIGNUM>;

inline BNCtxPtr createBNContext()
{
    BNCtxPtr bnContext(BN_CTX_new());
    if (!bnContext)
    {
        BOOST_THROW_EXCEPTION(CreateBNContextError());
    }
    return bnContext;
}

class BigNum
{
public:
    BigNum() : m_bn(BNPtr(BN_new(), BNDeleter())) {}
    BigNum(bcos::bytesConstRef const& _data, bool _signed = true) : BigNum()
    {
        fromBytes(_data, _signed);
    }
    BigNum(bcos::bytesConstRef const& _data, BigNum const& p, bool _signed) : BigNum()
    {
        fromBytesModP(_data, p, _signed);
    }

    explicit BigNum(int64_t const _value) : BigNum()
    {
        auto absV = _value > 0 ? _value : (-_value);
        auto ret = BN_set_word(m_bn.get(), absV);
        if (1 != ret)
        {
            BOOST_THROW_EXCEPTION(
                ConvertIntToBigNumError() << bcos::errinfo_comment(
                    "BN_hex2bn error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
        BN_set_negative(m_bn.get(), (_value >= 0 ? 0 : 1));
    }

    BigNum(s1024 const& _value) : BigNum()
    {
        bcos::bytes encodedData(128, 0);
        u1024 unsignedValue = u1024(_value);
        bool negative = false;
        if (_value < 0)
        {
            unsignedValue = u1024(-_value);
            negative = true;
        }
        bcos::toBigEndian(unsignedValue, encodedData);
        fromBigEndianBytes(encodedData.data(), encodedData.size(), negative);
    }

    ~BigNum() = default;

    // swap _data with m_bn
    void swap(BIGNUM* _data) { BN_swap(m_bn.get(), _data); }
    void copy(BIGNUM* _data) { BN_copy(_data, m_bn.get()); }
    int64_t getWord() const { return (int64_t)BN_get_word(m_bn.get()); }

    // convert bytes to BIGNUM
    void fromBytes(bcos::bytesConstRef const& _data, bool _signed = true)
    {
        if (_data.size() == 0)
        {
            BOOST_THROW_EXCEPTION(
                ConvertBytesToBigNumError() << bcos::errinfo_comment("Invalid BigNum data"));
        }
        if (_signed)
        {
            bool negative = false;
            if (_data[0] == 0xff)
            {
                negative = true;
            }
            fromBigEndianBytes((bcos::byte*)(_data.data() + 1), _data.size() - 1, negative);
            return;
        }
        fromBigEndianBytes((bcos::byte*)(_data.data()), _data.size(), false);
    }

    void fromBigEndianBytes(bcos::byte* _start, int64_t _size, bool _negative)
    {
        if (!BN_bin2bn(_start, _size, m_bn.get()))
        {
            BOOST_THROW_EXCEPTION(ConvertBytesToBigNumError());
        }
        if (_negative)
        {
            BN_set_negative(m_bn.get(), 1);
        }
    }

    void fromBytesModP(bcos::bytesConstRef const& _data, BigNum const& p, bool _signed = true) const
    {
        BigNum value(_data, _signed);
        auto bnContext = createBNContext();
        if (BN_nnmod(m_bn.get(), value.bn().get(), p.bn().get(), bnContext.get()) != 1)
        {
            BOOST_THROW_EXCEPTION(ConvertBytesToBigNumError());
        }
    }

    // convert BIGNUM to bytes
    void toBytes(bcos::bytes& _output, bool _signed = true) const
    {
        auto bytesLen = BN_num_bytes(m_bn.get());
        if (bytesLen < 0)
        {
            BOOST_THROW_EXCEPTION(
                BigNumGetBytesLenError() << bcos::errinfo_comment(
                    "BN_num_bytes error:" + std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
        auto bufferSize = _signed ? (bytesLen + 1) : bytesLen;
        _output.resize(bufferSize);
        OutputBuffer buffer{_output.data(), _output.size()};
        toOutputBuffer(&buffer, _signed);
    }

    // convert to s1024
    s1024 toS1024() const
    {
        auto bytesLen = BN_num_bytes(m_bn.get());
        bcos::bytes bigEndianBytes(bytesLen, 0);
        BN_bn2bin(m_bn.get(), (bcos::byte*)bigEndianBytes.data());
        auto result = bcos::fromBigEndian<s1024>(bigEndianBytes);
        if (BN_is_negative(m_bn.get()))
        {
            return s1024(0 - result);
        }
        return result;
    }

    void toOutputBuffer(OutputBuffer* _output, bool _signed = true) const
    {
        auto bytesLen = BN_num_bytes(m_bn.get());
        if (bytesLen < 0)
        {
            BOOST_THROW_EXCEPTION(
                BigNumGetBytesLenError() << bcos::errinfo_comment(
                    "BN_num_bytes error:" + std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
        auto bufferSize = _signed ? (bytesLen + 1) : bytesLen;
        if (_output->len < (uint64_t)bufferSize)
        {
            BOOST_THROW_EXCEPTION(
                NotEnoughBufferSize() << bcos::errinfo_comment(
                    "toOutputBuffer error for not enough buffer size, min buffer size is " +
                    std::to_string(bufferSize)));
        }
        _output->len = bufferSize;
        if (_signed)
        {
            if (!BN_is_negative(m_bn.get()))
            {
                *(_output->data) = 0x00;
            }
            else
            {
                *(_output->data) = 0xff;
            }
            BN_bn2bin(m_bn.get(), (bcos::byte*)_output->data + 1);
            return;
        }
        BN_bn2bin(m_bn.get(), (bcos::byte*)_output->data);
    }

    // invert: (m_bn)^-1 mod _p
    BigNum Invert(BigNum const& _p) const
    {
        auto bnContext = createBNContext();
        BigNum inversedNum;
        BN_mod_inverse(inversedNum.bn().get(), m_bn.get(), _p.bn().get(), bnContext.get());
        return inversedNum;
    }

    // m_bn + _value
    BigNum add(BIGNUM const* _v) const
    {
        BigNum result;
        add(result.bn().get(), _v);
        return result;
    }

    void add(BIGNUM* _result, BIGNUM const* _v) const
    {
        auto ret = BN_add(_result, m_bn.get(), _v);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(
                    BigNumAddError() << bcos::errinfo_comment(
                        "BN_add error:" + std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // (m_bn + _v) mod _m
    BigNum modAdd(BIGNUM const* _v, BIGNUM const* _m) const
    {
        BigNum result;
        auto ret = BN_mod_add_quick(result.bn().get(), m_bn.get(), _v, _m);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(BigNumAddError() << bcos::errinfo_comment(
                                          "BN_mod_add_quick error:" +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
        return result;
    }

    // sub
    BigNum sub(BIGNUM const* _subValue) const
    {
        BigNum result;
        auto ret = BN_sub(result.bn().get(), m_bn.get(), _subValue);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(
                    BigNumSubError() << bcos::errinfo_comment(
                        "BN_sub error:" + std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
        return result;
    }

    // multiply
    void mul(BIGNUM* _result, BIGNUM const* _value, BN_CTX* _ctx) const
    {
        auto ret = BN_mul(_result, m_bn.get(), _value, _ctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(
                    BigNumMultiError() << bcos::errinfo_comment(
                        "BN_mul error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // mud sub
    void modSub(BIGNUM* _result, BIGNUM const* _value, BIGNUM const* _p, BN_CTX* _ctx) const
    {
        BigNum result;
        auto ret = BN_mod_sub(_result, m_bn.get(), _value, _p, _ctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(BigNumAddError() << bcos::errinfo_comment(
                                          "BN_mod_sub error:" +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // mod multiply
    void modMul(BIGNUM* _result, BIGNUM const* _value, BIGNUM const* _p, BN_CTX* _ctx) const
    {
        auto ret = BN_mod_mul(_result, m_bn.get(), _value, _p, _ctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(BigNumMultiError() << bcos::errinfo_comment(
                                          "BN_mod_mul error: " +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // generate prime bn
    void generatePrime(unsigned int const _keyLength)
    {
        // Note: here set safe to 0 to improve the performance
        //       if set safe to 1, generate prime will cost multiple ms
        auto ret = BN_generate_prime_ex(m_bn.get(), _keyLength, 0, NULL, NULL, NULL);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(GeneratePrimeError() << bcos::errinfo_comment(
                                          "BN_generate_prime_ex error: " +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // gcd(m_bn, _q)
    void gcd(BIGNUM* _result, BIGNUM const* _q, BN_CTX* _bnctx) const
    {
        auto ret = BN_gcd(_result, m_bn.get(), _q, _bnctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(
                    BigNumGCDError() << bcos::errinfo_comment(
                        "BN_gcd error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // m_bn/_q
    void div(BIGNUM* _div, BIGNUM* _r, BIGNUM const* _q, BN_CTX* _bnctx) const
    {
        auto ret = BN_div(_div, _r, m_bn.get(), _q, _bnctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(
                    BigNumDivError() << bcos::errinfo_comment(
                        "BN_div error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // (m_bn)^p mod m
    BigNum modExp(const BIGNUM* _p, const BIGNUM* _m, BN_CTX* _ctx) const
    {
        BigNum result;
        auto ret = BN_mod_exp(result.bn().get(), m_bn.get(), _p, _m, _ctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(BigNumModExpError() << bcos::errinfo_comment(
                                          "BN_mod_exp error: " +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
        return result;
    }

    // m_bn ^p
    BigNum exp(const BIGNUM* _p, BN_CTX* _ctx) const
    {
        BigNum result;
        auto ret = BN_exp(result.bn().get(), m_bn.get(), _p, _ctx);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(
                    BigNumModExpError() << bcos::errinfo_comment(
                        "BN_exp error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
        return result;
    }

    int isBitSet(int n) { return BN_is_bit_set(m_bn.get(), n); }

    int cmp(const BIGNUM* b) const { return BN_cmp(m_bn.get(), b); }

    // set m_bn to _v
    void setWord(BN_ULONG _v)
    {
        auto ret = BN_set_word(m_bn.get(), _v);
        if (1 != ret)
            [[unlikely]]
            {
                BOOST_THROW_EXCEPTION(BigNumSetWordError() << bcos::errinfo_comment(
                                          "BN_set_word error: " +
                                          std::string(ERR_error_string(ERR_get_error(), NULL))));
            }
    }

    // divide m_bn  by _v
    void divConst(BN_ULONG w)
    {
        auto ret = BN_div_word(m_bn.get(), w);
        if ((BN_ULONG)-1 == ret)
        {
            BOOST_THROW_EXCEPTION(
                BigNumDivError() << bcos::errinfo_comment(
                    "BN_div_word error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
    }

    BigNum add(BigNum const& _num, BigNum const& _p) const
    {
        auto bnContext = createBNContext();
        BigNum addNum;
        BN_mod_add(addNum.bn().get(), m_bn.get(), _num.bn().get(), _p.bn().get(), bnContext.get());
        return addNum;
    }

    BigNum sub(BigNum const& _num, BigNum const& _p) const
    {
        auto bnContext = createBNContext();
        BigNum subNum;
        BN_mod_sub(subNum.bn().get(), m_bn.get(), _num.bn().get(), _p.bn().get(), bnContext.get());
        return subNum;
    }

    BigNum mul(BigNum const& _num, BigNum const& _p) const
    {
        auto bnContext = createBNContext();
        BigNum mulNum;
        BN_mod_mul(mulNum.bn().get(), m_bn.get(), _num.bn().get(), _p.bn().get(), bnContext.get());
        return mulNum;
    }
    BNPtr const& bn() const { return m_bn; }

private:
    BNPtr m_bn;
};

// generate random in range of (0, _n)
inline void randRange(BIGNUM* _r, const BIGNUM* _n)
{
    auto ret = BN_rand_range(_r, _n);
    if (1 != ret)
        [[unlikely]]
        {
            BOOST_THROW_EXCEPTION(BigNumRandError() << bcos::errinfo_comment(
                                      "BN_rand_range error: " +
                                      std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
}

// generate random with bits size
inline BigNum generateRand(int _bits)
{
    BigNum r;
    auto ret = BN_rand(r.bn().get(), _bits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
    if (1 != ret)
        [[unlikely]]
        {
            BOOST_THROW_EXCEPTION(
                BigNumRandError() << bcos::errinfo_comment(
                    "BN_rand error: " + std::string(ERR_error_string(ERR_get_error(), NULL))));
        }
    return r;
}

}  // namespace ppc::crypto