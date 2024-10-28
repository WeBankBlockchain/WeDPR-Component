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
 * @file OpenSSLPaillier.cpp
 * @author: yujiechen
 * @date 2023-08-04
 */
#include "OpenSSLPaillier.h"
#include "Common.h"
#include "ppc-framework/libwrapper/BigNum.h"
#include <bcos-utilities/DataConvertUtility.h>

using namespace ppc::homo;
using namespace ppc::crypto;
using namespace bcos;

// generate the keypair
KeyPair::UniquePtr OpenSSLPaillier::generateKeyPair(unsigned const _keyBits) const
{
    BigNum gcdResult;
    BigNum two;
    BigNum pMinus;
    BigNum qMinus;
    two.setWord(2);
    auto ctx = createBNContext();
    auto sk = std::make_shared<PaillierPrivateKey>();

    do
    {
        // generate prime p and q, ensure p != q, and p = 3 mod 4, q = 3 mod 4
        do
        {
            sk->p.generatePrime(_keyBits / 2);
        } while (!sk->p.isBitSet(1));
        do
        {
            sk->q.generatePrime(_keyBits / 2);
        } while (!sk->q.isBitSet(1) || (sk->q.cmp(sk->p.bn().get()) == 0));
        // calculate p-1 and q-1
        pMinus = sk->p.sub(BN_value_one());
        qMinus = sk->q.sub(BN_value_one());

        // calculate gcd(p-1, q-1), break when gcd(p-1, q-1) == 2
        pMinus.gcd(gcdResult.bn().get(), qMinus.bn().get(), ctx.get());
    } while (gcdResult.cmp(two.bn().get()));

    auto pk = std::make_shared<PaillierPublicKey>();
    pk->keyBits = _keyBits;
    sk->keyBits = _keyBits;

    // calculate n = p * q
    sk->p.mul(pk->n.bn().get(), sk->q.bn().get(), ctx.get());

    // generate random x
    BigNum x;
    randRange(x.bn().get(), pk->n.bn().get());
    // calculate h = -x^2
    x.mul(pk->h.bn().get(), x.bn().get(), ctx.get());
    BN_set_negative(pk->h.bn().get(), 1);
    //// precompute
    pk->precompute();

    /// calculate secret key
    // lambda = (p - 1) * (q-1)/2
    pMinus.mul(sk->lambda.bn().get(), qMinus.bn().get(), ctx.get());
    sk->lambda.divConst(2);
    // precompute
    sk->precompute();
    // return the paillier keypair
    return std::make_unique<OpenSSLPaillierKeyPair>(std::move(sk), std::move(pk));
}


/// optimize (_base^_exp)mod(n^2) using CRT
/// refer to: https://zhuanlan.zhihu.com/p/420503254
BigNum OpenSSLPaillier::powerModSqrtCrt(
    BigNum const& _base, BigNum const& _exp, OpenSSLPaillierKeyPair* _keyPair) const
{
    auto sk = (PaillierPrivateKey*)(_keyPair->sk());
    auto pk = (PaillierPublicKey*)(_keyPair->pk());

    auto ctx = createBNContext();
    // expp = _exp mod (p^2 - p)
    BigNum expp;
    _exp.div(NULL, expp.bn().get(), sk->pOrderSqrt.bn().get(), ctx.get());
    // baseP = _base mod p^2
    BigNum baseP;
    _base.div(NULL, baseP.bn().get(), sk->pSqrt.bn().get(), ctx.get());
    // xp = baseP ^ expp mod p^2
    auto xp = baseP.modExp(expp.bn().get(), sk->pSqrt.bn().get(), ctx.get());

    // expq = _exp mod (q^2 - q)
    BigNum expq;
    _exp.div(NULL, expq.bn().get(), sk->qOrderSqrt.bn().get(), ctx.get());
    // baseQ = _base mod q^2
    BigNum baseQ;
    _base.div(NULL, baseQ.bn().get(), sk->qSqrt.bn().get(), ctx.get());
    // xq = baseQ ^ expq mod q^2
    auto xq = baseQ.modExp(expq.bn().get(), sk->qSqrt.bn().get(), ctx.get());

    // crt to calculate (_base^_exp)mod(n^2)
    // h = ((xp - xq)*(qSqrtInv)) mod p^2
    // _base^exp = xq + h * q^2
    auto result = xp.sub(xq.bn().get());
    BigNum tmp;
    result.modMul(tmp.bn().get(), sk->qSqrtInverse.bn().get(), sk->pSqrt.bn().get(), ctx.get());
    tmp.mul(tmp.bn().get(), sk->qSqrt.bn().get(), ctx.get());

    return tmp.modAdd(xq.bn().get(), pk->nSqrt.bn().get());
}

void OpenSSLPaillier::encryptImpl(OutputBuffer* _resultBuffer, OutputBuffer* _rBuffer,
    BIGNUM const* _m, OpenSSLPaillierKeyPair* _keyPair, void* _pk) const
{
    auto pk = (PaillierPublicKey*)_pk;
    // encode _m to support negative case
    SignedNumberCodec codec(pk->n);
    auto m = codec.encode(_m);
    // calculate: g^m
    // Note: since g=(n+1) when generate keypair, g^m can be speed up to [(mn + 1) mod n^2]
    BigNum tmp;
    auto ctx = createBNContext();
    m.mul(tmp.bn().get(), pk->n.bn().get(), ctx.get());
    auto g_power_m = tmp.modAdd(BN_value_one(), pk->nSqrt.bn().get());

    // generate random r with ceil(|n|/2) bits
    // rBits is ceil(keyBits)
    auto rBits = pk->keyBits % 2 ? ((pk->keyBits / 2) + 1) : (pk->keyBits / 2);
    auto r = generateRand(rBits);
    // convert r to buffer
    if (_rBuffer)
    {
        r.toOutputBuffer(_rBuffer, false);
    }
    // calculate r^n mod n^2, namely (h_s^r mod n^2)
    BigNum r_power_n;
    // with crt optimization
    if (_keyPair && _keyPair->sk())
    {
        r_power_n = powerModSqrtCrt(pk->h_s, r, _keyPair);
    }
    else
    {
        // without crt optmization
        r_power_n = pk->h_s.modExp(r.bn().get(), pk->nSqrt.bn().get(), ctx.get());
    }
    // (g^m) * (r^n) mod n^2
    BigNum result;
    g_power_m.modMul(result.bn().get(), r_power_n.bn().get(), pk->nSqrt.bn().get(), ctx.get());
    result.toOutputBuffer(_resultBuffer, true);
}

// encrypt the plain data
void OpenSSLPaillier::encrypt_with_crt(
    OutputBuffer* _cipherBytes, OutputBuffer* _rBuffer, BIGNUM const* _v, void* _keyPair) const
{
    if (!_cipherBytes)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "encrypt_with_crt error for Invalid OutputBuffer"));
    }
    if (!_v)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException()
                              << bcos::errinfo_comment("encrypt_with_crt error for invalid _v"));
    }
    if (!_keyPair)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "encrypt_with_crt error for invalid _keyPair"));
    }
    auto keyPair = (OpenSSLPaillierKeyPair*)_keyPair;
    if (!keyPair->pk())
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "encrypt_with_crt error for invalid publicKey"));
    }
    encryptImpl(_cipherBytes, _rBuffer, _v, keyPair, keyPair->pk());
}

// decrypt the cipher data
BigNum OpenSSLPaillier::decrypt(bcos::bytesConstRef const& _cipherData, void* _keyPair) const
{
    if (!_keyPair)
    {
        BOOST_THROW_EXCEPTION(
            InvalidHomoKeyPair() << bcos::errinfo_comment("decrypt error for empty keypair"));
    }
    BigNum cipher(_cipherData);
    auto keyPair = (OpenSSLPaillierKeyPair*)_keyPair;
    auto sk = (PaillierPrivateKey*)(keyPair->sk());
    if (!sk)
    {
        BOOST_THROW_EXCEPTION(
            InvalidHomoKeyPair() << bcos::errinfo_comment("decrypt error for empty sk"));
    }
    auto pk = (PaillierPublicKey*)(keyPair->pk());
    if (!pk)
    {
        BOOST_THROW_EXCEPTION(
            InvalidHomoKeyPair() << bcos::errinfo_comment("decrypt error for empty pk"));
    }
    auto ctx = createBNContext();
    // calculate x = (cipher^lambda mod n^2)
    auto x = powerModSqrtCrt(cipher, sk->lambda, keyPair);
    // calculate (x-1)/n
    BigNum result;
    result = x.sub(BN_value_one());
    result.div(result.bn().get(), NULL, pk->n.bn().get(), ctx.get());

    // result * lambdaInverse mod n
    auto lambdaInverse = sk->lambda.Invert(pk->n);
    result.modMul(result.bn().get(), lambdaInverse.bn().get(), pk->n.bn().get(), ctx.get());
    // decode result to support negative case
    SignedNumberCodec codec(pk->n);
    return codec.decode(result);
}

void OpenSSLPaillier::addImpl(OutputBuffer* _resultBytes, ppc::crypto::BigNum const& _cipher1,
    ppc::crypto::BigNum const& _cipher2, void* _publicKey) const
{
    // Note: the keypair contain only the public key
    auto pk = (PaillierPublicKey*)(_publicKey);
    // c1 * c2 mod n*n
    auto ctx = createBNContext();
    BigNum result;
    _cipher1.modMul(result.bn().get(), _cipher2.bn().get(), pk->nSqrt.bn().get(), ctx.get());
    bcos::bytes cipherResult;
    result.toOutputBuffer(_resultBytes, true);
}

void OpenSSLPaillier::add(OutputBuffer* _result, bcos::bytesConstRef const& _cipherData1,
    bcos::bytesConstRef const& _cipherData2, void* _publicKey) const
{
    if (!_result)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier add error for Invalid OutputBuffer"));
    }
    if (!_publicKey)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier add error for invalid public key"));
    }
    BigNum cipher1(_cipherData1);
    BigNum cipher2(_cipherData2);
    addImpl(_result, cipher1, cipher2, _publicKey);
}

void OpenSSLPaillier::sub(OutputBuffer* _resultBytes, bcos::bytesConstRef const& _cipherData1,
    bcos::bytesConstRef const& _cipherData2, void* _publicKey) const
{
    if (!_resultBytes)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier sub error for Invalid OutputBuffer"));
    }
    if (!_publicKey)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier sub error for invalid public key"));
    }
    // Note: the keypair contain only the public key
    auto pk = (PaillierPublicKey*)(_publicKey);
    BigNum cipher1(_cipherData1);
    BigNum cipher2(_cipherData2);
    // ((c1 * (c2^-1)) mod n*n
    auto cipher2Invert = cipher2.Invert(pk->nSqrt);
    return addImpl(_resultBytes, cipher1, cipher2Invert, _publicKey);
}

void OpenSSLPaillier::scalaMul(OutputBuffer* _resultBytes, BIGNUM const* _value,
    bcos::bytesConstRef const& _cipherData, void* _publicKey) const
{
    if (!_resultBytes)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier scalaMul error for invalid OutputBuffer"));
    }
    if (!_value)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier scalaMul error for invalid input v"));
    }
    if (!_publicKey)
    {
        BOOST_THROW_EXCEPTION(OpenSSLPaillierException() << bcos::errinfo_comment(
                                  "paillier scalaMul error for invalid public key"));
    }
    // Note: the keypair contain only the public key
    auto pk = (PaillierPublicKey*)(_publicKey);
    // encode to support negative v
    SignedNumberCodec codec(pk->n);
    auto v = codec.encode(_value);

    BigNum cipher(_cipherData);

    // cipher^v mod (n^2)
    auto ctx = createBNContext();
    auto result = cipher.modExp(v.bn().get(), pk->nSqrt.bn().get(), ctx.get());
    result.toOutputBuffer(_resultBytes, true);
}