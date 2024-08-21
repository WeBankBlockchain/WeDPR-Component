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
 * @file IhcImpl.cpp
 * @author: asherli
 * @date 2023-11-23
 */
#include "IhcImpl.h"
#include "ppc-tools/src/codec/CodecUtility.h"

using namespace ppc::homo;
using namespace ppc::crypto;

// generate key according to given mode
BigNum IhcImpl::generateKeyImpl() const
{
    BigNum key;
    do
    {
        randRange(key.bn().get(), m_codec->n().bn().get());
    } while (BN_is_zero(key.bn().get()));
    return key;
}

void IhcImpl::generateKey(OutputBuffer* _result) const
{
    if (_result->len < keyBytes())
    {
        BOOST_THROW_EXCEPTION(
            IhcException() << bcos::errinfo_comment(
                "IHC error for un-enough key buffer, current len: " + std::to_string(_result->len) +
                ", min key len: " + std::to_string(keyBytes())));
    }
    auto key = generateKeyImpl();
    key.toOutputBuffer(_result, false);
}

bcos::bytes IhcImpl::generateKey() const
{
    bcos::bytes result(keyBytes());
    generateKeyImpl().toBytes(result, false);
    return result;
}
// encrypt the given value to cipher using given key
void IhcImpl::encrypt(
    OutputBuffer* _cipher, bcos::bytesConstRef const& _key, BIGNUM const* _value) const
{
    auto v = m_codec->encode(_value);

    BigNum lastRoundV;
    randRange(lastRoundV.bn().get(), m_codec->n().bn().get());
    BigNum key(_key, false);
    auto ctx = createBNContext();
    for (int i = 0; i < m_iterRound; ++i)
    {
        BigNum tmp;
        v.modMul(tmp.bn().get(), key.bn().get(), m_codec->n().bn().get(), ctx.get());
        tmp.modSub(tmp.bn().get(), lastRoundV.bn().get(), m_codec->n().bn().get(), ctx.get());
        lastRoundV = v;
        v = tmp;
    }
    // c1: v
    auto pEnd = _cipher->data + _cipher->len;
    auto pBuffer = encodeBigNum(_cipher->data, pEnd, v);
    // c2: lastRoundV
    pBuffer = encodeBigNum(pBuffer, pEnd, lastRoundV);
    _cipher->len = (pBuffer - _cipher->data);
}

// decrypt the given cipher to value using given key
ppc::crypto::BigNum IhcImpl::decrypt(
    bcos::bytesConstRef const& _key, bcos::bytesConstRef const& _cipher) const
{
    BigNum lastRoundV;
    auto offset = decodeBigNum(lastRoundV, _cipher.data(), _cipher.size(), 0);
    BigNum v;
    decodeBigNum(v, _cipher.data(), _cipher.size(), offset);
    BigNum key(_key, false);
    auto ctx = createBNContext();
    for (int i = 0; i < m_iterRound - 1; ++i)
    {
        BigNum tmp;
        v.modMul(tmp.bn().get(), key.bn().get(), m_codec->n().bn().get(), ctx.get());
        tmp.modSub(tmp.bn().get(), lastRoundV.bn().get(), m_codec->n().bn().get(), ctx.get());
        lastRoundV = v;
        v = tmp;
    }
    return m_codec->decode(v);
}

// ihc add
void IhcImpl::arithmeticImpl(OutputBuffer* _addResult, bcos::bytesConstRef const& _c1,
    bcos::bytesConstRef const& _c2, ArithmeticType _type) const
{
    // c1
    BigNum leftV;
    auto offset = decodeBigNum(leftV, _c1.data(), _c1.size(), 0);
    BigNum leftLastRoundV;
    decodeBigNum(leftLastRoundV, _c1.data(), _c1.size(), offset);
    // c2
    BigNum rightV;
    offset = decodeBigNum(rightV, _c2.data(), _c2.size(), 0);
    BigNum rightLastRoundV;
    decodeBigNum(rightLastRoundV, _c2.data(), _c2.size(), offset);
    // c1 + c2
    BigNum resultV;
    BigNum resultLastRoundV;
    auto ctx = createBNContext();
    switch (_type)
    {
    case IhcImpl::ArithmeticType::ADD:
    {
        resultV = leftV.modAdd(rightV.bn().get(), m_codec->n().bn().get());
        resultLastRoundV =
            leftLastRoundV.modAdd(rightLastRoundV.bn().get(), m_codec->n().bn().get());
        break;
    }
    case IhcImpl::ArithmeticType::SUB:
    {
        leftV.modSub(resultV.bn().get(), rightV.bn().get(), m_codec->n().bn().get(), ctx.get());
        leftLastRoundV.modSub(resultLastRoundV.bn().get(), rightLastRoundV.bn().get(),
            m_codec->n().bn().get(), ctx.get());
        break;
    }
    default:
        BOOST_THROW_EXCEPTION(IhcException() << bcos::errinfo_comment(
                                  "Unsupported ArithmeticType: " + std::to_string((int)_type)));
    }
    auto pEnd = _addResult->data + _addResult->len;
    auto pBuffer = encodeBigNum(_addResult->data, pEnd, resultV);
    pBuffer = encodeBigNum(pBuffer, pEnd, resultLastRoundV);
    _addResult->len = (pBuffer - _addResult->data);
}

void IhcImpl::add(
    OutputBuffer* _addResult, bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const
{
    arithmeticImpl(_addResult, _c1, _c2, IhcImpl::ArithmeticType::ADD);
}

void IhcImpl::sub(
    OutputBuffer* _subResult, bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const
{
    arithmeticImpl(_subResult, _c1, _c2, IhcImpl::ArithmeticType::SUB);
}

// ihc scalaMul
void IhcImpl::scalaMul(
    OutputBuffer* _mulResult, BIGNUM const* _value, bcos::bytesConstRef const& _cipher) const
{
    // c1
    BigNum leftV;
    auto offset = decodeBigNum(leftV, _cipher.data(), _cipher.size(), 0);
    BigNum leftLastRoundV;
    decodeBigNum(leftLastRoundV, _cipher.data(), _cipher.size(), offset);
    BigNum resultV;
    BigNum resultLastRoundV;
    auto ctx = createBNContext();

    leftV.modMul(resultV.bn().get(), _value, m_codec->n().bn().get(), ctx.get());
    leftLastRoundV.modMul(resultLastRoundV.bn().get(), _value, m_codec->n().bn().get(), ctx.get());

    auto pEnd = _mulResult->data + _mulResult->len;
    auto pBuffer = encodeBigNum(_mulResult->data, pEnd, resultV);
    pBuffer = encodeBigNum(pBuffer, pEnd, resultLastRoundV);
    _mulResult->len = (pBuffer - _mulResult->data);
}