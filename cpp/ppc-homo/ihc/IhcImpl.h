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
 * @file IhcImpl.h
 * @author: asherli
 * @date 2023-11-23
 */

#pragma once
#include "ppc-framework/Common.h"
#include "ppc-framework/crypto/Ihc.h"
#include "ppc-homo/codec/SignedNumberCodec.h"
namespace ppc::homo
{
DERIVE_PPC_EXCEPTION(IhcException);
// Ihc implementation:
class IhcImpl : public Ihc
{
public:
    using Ptr = std::shared_ptr<IhcImpl>;
    IhcImpl(int _mode, int _iterRound) : m_mode(_mode), m_iterRound(_iterRound)
    {
        switch (_mode)
        {
        case (int)Ihc::IhcMode::IHC_128:
            m_keyBits = 128;
            break;
        case (int)Ihc::IhcMode::IHC_256:
            m_keyBits = 256;
            break;
        default:
            BOOST_THROW_EXCEPTION(IhcException() << bcos::errinfo_comment(
                                      "Unsupported mode, only support IHC_128/IHC_256 now"));
        }
        m_iterRound = 16;
        m_codec = std::make_shared<SignedNumberCodec>(m_keyBits);
    }
    ~IhcImpl() override {}

    // generate key according to given mode
    bcos::bytes generateKey() const override;
    void generateKey(OutputBuffer* _result) const override;

    // encrypt the given value to cipher using given key
    void encrypt(OutputBuffer* _cipher, bcos::bytesConstRef const& _key,
        BIGNUM const* _value) const override;

    bcos::bytes encrypt(bcos::bytesConstRef const& _key, BIGNUM const* _value) const override
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer resultCipher{result.data(), result.size()};
        encrypt(&resultCipher, _key, _value);
        result.resize(resultCipher.len);
        return result;
    }

    // decrypt the given cipher to value using given key
    ppc::crypto::BigNum decrypt(
        bcos::bytesConstRef const& _key, bcos::bytesConstRef const& _cipher) const override;

    // ihc add
    void add(OutputBuffer* _addResult, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2) const override;
    bcos::bytes add(bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const override
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer buffer{result.data(), result.size()};
        add(&buffer, _c1, _c2);
        result.resize(buffer.len);
        return result;
    }
    //  ihc sub
    void sub(OutputBuffer* _subResult, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2) const override;

    bcos::bytes sub(bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const override
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer buffer{result.data(), result.size()};
        sub(&buffer, _c1, _c2);
        result.resize(buffer.len);
        return result;
    }
    // ihc scalaMul
    void scalaMul(OutputBuffer* _mulResult, BIGNUM const* _value,
        bcos::bytesConstRef const& _cipher) const override;

    bcos::bytes scalaMul(BIGNUM const* _value, bcos::bytesConstRef const& _cipher) const override
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer buffer{result.data(), result.size()};
        scalaMul(&buffer, _value, _cipher);
        result.resize(buffer.len);
        return result;
    }

    unsigned int keyBits() const override { return m_keyBits; }
    unsigned int keyBytes() const override { return (m_keyBits + 7) / 8; }
    uint64_t cipherBytes() const override { return 6 + (m_keyBits * 2) / 8; }

    int mode() const { return m_mode; }

private:
    enum class ArithmeticType
    {
        ADD,
        SUB,
    };
    void arithmeticImpl(OutputBuffer* _addResult, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2, ArithmeticType _type) const;

    ppc::crypto::BigNum generateKeyImpl() const;

private:
    int m_mode;
    int m_iterRound;
    unsigned int m_keyBits;
    SignedNumberCodec::Ptr m_codec;
};
}  // namespace ppc::homo
