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
 * @file FastOre.cpp
 * @author: shawnhe
 * @date 2023-08-18
 */
#include "FastOre.h"
#include "../Common.h"

using namespace ppc;
using namespace ppc::crypto;

// cipher encoded with hex
std::string FastOre::encrypt4String(
    bcos::bytesConstRef const& _sk, const std::string& _plaintext) const
{
    bcos::bytes cipher(estimatedCipherSize(_plaintext.size(), false));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    encrypt4String(
        &cipherBuffer, _sk, bcos::bytesConstRef((bcos::byte*)_plaintext.data(), _plaintext.size()));
    return bcos::toHex(bcos::bytes{cipher.begin(), cipher.end()});
}

void FastOre::encrypt4String(OutputBuffer* _cipher, bcos::bytesConstRef const& _sk,
    bcos::bytesConstRef const& _plaintext) const
{
    if (_plaintext.size() == 0)
    {
        return;
    }
    auto estimatedSize = estimatedCipherSize(_plaintext.size(), false);
    if (_cipher->len < estimatedSize)
    {
        BOOST_THROW_EXCEPTION(
            FastOreException() << bcos::errinfo_comment(
                "FastOre4String encrypt failed for unenough cipher buffer, expect at least " +
                std::to_string(estimatedSize)));
    }

    // enc the first byte
    bcos::bytes cipher0(CIPHER_BLOCK_SIZE);
    auto tempCipher0 = encWithTruncation(
        _sk, bcos::bytesConstRef((const unsigned char*)cipher0.data(), CIPHER_BLOCK_SIZE));
    uint16_t sum0 = (uint8_t)tempCipher0[1] + (uint8_t)_plaintext[0];
    _cipher->data[0] = (uint8_t)tempCipher0[0] + sum0 / 256;
    _cipher->data[1] = sum0 % 256;

    // enc the other bytes
    for (uint64_t i = 1; i < _plaintext.size(); i++)
    {
        auto tempCipher = encWithTruncation(
            _sk, bcos::bytesConstRef((const unsigned char*)_cipher->data, i * CIPHER_BLOCK_SIZE));
        uint16_t sum = (uint8_t)tempCipher[1] + (uint8_t)_plaintext[i];
        _cipher->data[i * CIPHER_BLOCK_SIZE] = (uint8_t)tempCipher[0] + sum / 256;
        _cipher->data[i * CIPHER_BLOCK_SIZE + 1] = sum % 256;
    }
}

// cipher encoded with hex
std::string FastOre::decrypt4String(
    bcos::bytesConstRef const& _key, const std::string& _ciphertext) const
{
    bcos::bytes cipher = bcos::fromHex(_ciphertext);
    std::string plain;
    plain.resize(estimatedPlainSize(_ciphertext.size(), true));
    OutputBuffer resultBuffer{(bcos::byte*)plain.data(), plain.size()};
    decrypt4String(&resultBuffer, _key, bcos::ref(cipher));

    return plain;
}

void FastOre::decrypt4String(
    OutputBuffer* _plain, bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& _cipher) const
{
    if (_cipher.size() < CIPHER_BLOCK_SIZE)
    {
        BOOST_THROW_EXCEPTION(
            FastOreException() << bcos::errinfo_comment(
                "FastOre4String decrypt failed for invalid cipher, min cipher size is: " +
                std::to_string(CIPHER_BLOCK_SIZE)));
    }
    auto blockSize = estimatedPlainSize(_cipher.size(), false);
    if (_plain->len < blockSize)
    {
        BOOST_THROW_EXCEPTION(
            FastOreException() << bcos::errinfo_comment(
                "FastOre4String decrypt failed for unenough plain buffer, expect at least: " +
                std::to_string(blockSize)));
    }

    // dec the first byte
    bcos::bytes cipher0(CIPHER_BLOCK_SIZE);
    auto tempCipher0 = encWithTruncation(
        _sk, bcos::bytesConstRef((const unsigned char*)cipher0.data(), CIPHER_BLOCK_SIZE));
    int diff0 = (uint8_t)_cipher[1] - (uint8_t)tempCipher0[1];
    if (diff0 < 0)
    {
        diff0 += 256;
    }
    _plain->data[0] = diff0;

    // dec the other bytes
    for (uint64_t i = 1; i < blockSize; i++)
    {
        auto tempCipher = encWithTruncation(
            _sk, bcos::bytesConstRef((const unsigned char*)_cipher.data(), i * CIPHER_BLOCK_SIZE));
        int diff = (uint8_t)_cipher[i * CIPHER_BLOCK_SIZE + 1] - (uint8_t)tempCipher[1];
        if (diff < 0)
        {
            diff += 256;
        }
        _plain->data[i] = diff;
    }
}

// cipher encoded with base64
std::string FastOre::encrypt4Integer(bcos::bytesConstRef const& _sk, const int64_t& _plain) const
{
    bcos::bytes cipher(estimatedCipherSize(sizeof(_plain), false));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    encrypt4Integer(&cipherBuffer, _sk, _plain);
    return bcos::toHex(bcos::bytes{cipher.begin(), cipher.end()});
}

void FastOre::encrypt4Integer(
    OutputBuffer* _cipher, bcos::bytesConstRef const& _sk, const int64_t& _plain) const
{
    int64_t plain = 0;
    OutputBuffer plainB{(bcos::byte*)&plain, sizeof(plain)};
    formatNumberPlain(&plainB, _plain);
    encrypt4String(_cipher, _sk, bcos::bytesConstRef((bcos::byte*)plainB.data, plainB.len));
}

// cipher encoded with base64
int64_t FastOre::decrypt4Integer(bcos::bytesConstRef const& _key, const std::string& _cipher) const
{
    bcos::bytes cipher = bcos::fromHex(_cipher);
    int64_t plain = 0;
    decrypt4Integer(&plain, _key, bcos::ref(cipher));
    return plain;
}

void FastOre::decrypt4Integer(
    int64_t* _plain, bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& cipher) const
{
    int64_t plain = 0;
    OutputBuffer resultBuffer{(bcos::byte*)&plain, sizeof(plain)};
    decrypt4String(&resultBuffer, _sk, cipher);
    *_plain = recoverNumberPlain(resultBuffer);
}

// cipher encoded with hex
std::string FastOre::encrypt4Float(bcos::bytesConstRef const& _sk, const float50& _plain) const
{
    bcos::bytes cipher(estimatedFloatCipherSize(_plain.str().size(), false));
    OutputBuffer cipherBuffer{cipher.data(), cipher.size()};
    encrypt4Float(&cipherBuffer, _sk, _plain);
    return bcos::toHex(bcos::bytes{cipher.begin(), cipher.begin() + cipherBuffer.len});
}

void FastOre::encrypt4Float(
    OutputBuffer* _cipher, bcos::bytesConstRef const& _sk, const float50& _plain) const
{
    OreFloatingNumber ofn(_plain);
    encrypt4String(
        _cipher, _sk, bcos::bytesConstRef((bcos::byte*)&ofn.integerPart, sizeof(ofn.integerPart)));

    if (!ofn.decimalPart.empty())
    {
        OutputBuffer decimalCipherBuffer{
            _cipher->data + sizeof(ofn.integerPart) * CIPHER_BLOCK_SIZE,
            ofn.decimalPart.size() * CIPHER_BLOCK_SIZE};
        encrypt4String(&decimalCipherBuffer, _sk,
            bcos::bytesConstRef((bcos::byte*)ofn.decimalPart.data(), ofn.decimalPart.size()));
    }
    _cipher->len = (sizeof(ofn.integerPart) + ofn.decimalPart.size()) * CIPHER_BLOCK_SIZE;
}

// cipher encoded with base64
float50 FastOre::decrypt4Float(bcos::bytesConstRef const& _sk, const std::string& _cipher) const
{
    bcos::bytes cipher = bcos::fromHex(_cipher);
    return decrypt4Float(_sk, bcos::ref(cipher));
}

float50 FastOre::decrypt4Float(
    bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& _cipher) const
{
    OreFloatingNumber ofn;
    OutputBuffer integerOut{(bcos::byte*)&ofn.integerPart, sizeof(ofn.integerPart)};
    auto integerCipherSize = sizeof(ofn.integerPart) * CIPHER_BLOCK_SIZE;
    decrypt4String(&integerOut, _sk, bcos::bytesConstRef(_cipher.data(), integerCipherSize));

    if (_cipher.size() > integerCipherSize)
    {
        auto decimalSize = (_cipher.size() - integerCipherSize) / CIPHER_BLOCK_SIZE;
        ofn.decimalPart.resize(decimalSize);
        OutputBuffer decimalOut{(bcos::byte*)ofn.decimalPart.data(), decimalSize};
        decrypt4String(&decimalOut, _sk,
            bcos::bytesConstRef(
                _cipher.data() + integerCipherSize, decimalSize * CIPHER_BLOCK_SIZE));
    }
    return ofn.value();
}

int FastOre::compare(const std::string& _ciphertext0, const std::string& _ciphertext1) const
{
    return _ciphertext0.compare(_ciphertext1);
}

int FastOre::compare(InputBuffer const* c1, InputBuffer const* c2) const
{
    std::string str1{c1->data, c1->data + c1->len};
    std::string str2{c2->data, c2->data + c2->len};

    return str1.compare(str2);
}
