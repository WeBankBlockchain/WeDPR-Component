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
 * @file OpenSSLPaillierKeyPair.cpp
 * @author: yujiechen
 * @date 2023-08-07
 */
#include "OpenSSLPaillierKeyPair.h"

using namespace ppc::homo;
using namespace ppc;
using namespace ppc::crypto;

// serialize the sk
int PaillierPrivateKey::serialize(bcos::byte* _encodedData, unsigned int _dataLen) const
{
    auto pEnd = _encodedData + _dataLen;
    // keyBits
    auto pBuffer = encodeInteger(_encodedData, pEnd, keyBits);
    // lambda
    pBuffer = encodeBigNum(pBuffer, pEnd, lambda);
    // p
    pBuffer = encodeBigNum(pBuffer, pEnd, p);
    // q
    pBuffer = encodeBigNum(pBuffer, pEnd, q);
    return (pBuffer - _encodedData);
}

// serialize the pk
int PaillierPublicKey::serialize(bcos::byte* _encodedData, unsigned int _dataLen) const
{
    auto pEnd = _encodedData + _dataLen;
    // encode the keyBits
    auto pBuffer = encodeInteger(_encodedData, pEnd, keyBits);
    // n
    pBuffer = encodeBigNum(pBuffer, pEnd, n);
    // h
    pBuffer = encodeBigNum(pBuffer, pEnd, h);
    return (pBuffer - _encodedData);
}

// deserialize the sk
void PaillierPrivateKey::deserialize(bcos::byte const* _sk, unsigned int _len)
{
    // keyBits
    auto offset = decodeInteger(keyBits, _sk, _len, 0);
    // lambda
    offset = decodeBigNum(lambda, _sk, _len, offset);
    // p
    offset = decodeBigNum(p, _sk, _len, offset);
    // q
    offset = decodeBigNum(q, _sk, _len, offset);
}

// deserialize the pk
void PaillierPublicKey::deserialize(bcos::byte const* _pk, unsigned int _len)
{
    // keyBits
    auto offset = decodeInteger(keyBits, _pk, _len, 0);
    // n
    offset = decodeBigNum(n, _pk, _len, offset);
    // h
    offset = decodeBigNum(h, _pk, _len, offset);
}