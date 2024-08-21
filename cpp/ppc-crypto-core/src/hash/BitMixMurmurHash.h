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
 * @file BitMixMurmurHash.h
 * @author: yujiechen
 * @date 2022-10-25
 */
#pragma once
#include <bcos-utilities/Common.h>
#include <boost/core/ignore_unused.hpp>
namespace ppc::crypto
{
class BitMixMurmurHash
{
public:
    BitMixMurmurHash() = default;
    ~BitMixMurmurHash() = default;

    template <typename T>
    uint64_t hash(T const& _key, unsigned _hashBitsLen, uint32_t _seed = 32) const
    {
        // Note: _seed is unused when the key is integer
        boost::ignore_unused(_seed);
        static_assert(std::is_same<std::string, T>::value ||
                          std::is_same<std::string_view, T>::value ||
                          std::is_same<bcos::bytes, T>::value ||
                          std::is_same<bcos::bytesConstRef, T>::value ||
                          std::is_same<int8_t, T>::value || std::is_same<int16_t, T>::value ||
                          std::is_same<int32_t, T>::value || std::is_same<int64_t, T>::value ||
                          std::is_same<uint8_t, T>::value || std::is_same<uint16_t, T>::value ||
                          std::is_same<uint32_t, T>::value || std::is_same<uint64_t, T>::value,
            "only support string/string_view/bytes/bytesConstRef/int8_t to int64_t/uint8_t to "
            "uint64_t");
        // the bytes type
        if constexpr (std::is_same<T, std::string>::value ||
                      std::is_same<T, std::string_view>::value ||
                      std::is_same<T, bcos::bytes>::value ||
                      std::is_same<T, bcos::bytesConstRef>::value)
        {
            return hashData((const unsigned char*)_key.data(), _key.size(), _hashBitsLen, _seed);
        }
        else
        {
            // the int type
            return (_hashBitsLen <= 32 ? (hash32N(_key)) : hash64N(_key));
        }
    }

protected:
    uint64_t hashData(
        const unsigned char* key, uint32_t len, unsigned _hashBitsLen, uint32_t _seed) const
    {
        return (_hashBitsLen <= 32 ? (hash32(key, len, _seed)) : hash64(key, len, _seed));
    }

    inline uint64_t hash64N(uint64_t _value) const
    {  // Bit mix from MurmurHash64/CLHash
        _value ^= _value >> 33;
        _value *= 0xff51afd7ed558ccdULL;
        _value ^= _value >> 33;
        _value *= 0xc4ceb9fe1a85ec53ULL;
        _value ^= _value >> 33;
        return _value;
    }

    inline uint32_t hash32N(uint32_t _value) const
    {  // Bit mix from MurmurHash32
        _value ^= _value >> 16;
        _value *= 0x85ebca6b;
        _value ^= _value >> 13;
        _value *= 0xc2b2ae35;
        _value ^= _value >> 16;
        return _value;
    }

    // Note: when generating 32bits hash, there is a high probability of a collision
    //       so we default to generate 64bits hash
    uint32_t hash32(const unsigned char* key, uint32_t len, uint32_t seed) const
    {
        uint32_t c1 = 0xcc9e2d51;
        uint32_t c2 = 0x1b873593;
        uint32_t r1 = 15;
        uint32_t r2 = 13;
        uint32_t m = 5;
        uint32_t n = 0xe6546b64;
        uint32_t h = 0;
        uint32_t k = 0;
        uint8_t* d = (uint8_t*)key;  // 32 bit extract from `key'
        const uint32_t* chunks = NULL;
        const uint8_t* tail = NULL;  // tail - last 8 bytes
        int i = 0;
        int l = len / 4;  // chunk length

        h = seed;

        chunks = (const uint32_t*)(d + l * 4);  // body
        tail = (const uint8_t*)(d + l * 4);     // last 8 byte chunk of `key'

        // for each 4 byte chunk of `key'
        for (i = -l; i != 0; ++i)
        {
            // next 4 byte chunk of `key'
            k = chunks[i];

            // encode next 4 byte chunk of `key'
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;

            // append to hash
            h ^= k;
            h = (h << r2) | (h >> (32 - r2));
            h = h * m + n;
        }
        k = 0;
        // remainder
        switch (len & 3)
        {  // `len % 4'
        case 3:
            k ^= (tail[2] << 16);
            break;
        case 2:
            k ^= (tail[1] << 8);
            break;
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;
            h ^= k;
            break;
        default:  // unreachable-branch
            break;
        }

        h ^= len;

        h ^= (h >> 16);
        h *= 0x85ebca6b;
        h ^= (h >> 13);
        h *= 0xc2b2ae35;
        h ^= (h >> 16);

        return h;
    }

    uint64_t hash64(const unsigned char* key, const int len, const uint32_t seed) const
    {
        const uint8_t* data = (const uint8_t*)key;
        const int nblocks = len / 16;
        int i;

        uint64_t h1 = seed;
        uint64_t h2 = seed;

        uint64_t c1 = 0x87c37b91114253d5;
        uint64_t c2 = 0x4cf5ad432745937f;
        const uint64_t* blocks = (const uint64_t*)(data);

        for (i = 0; i < nblocks; i++)
        {
            uint64_t k1 = blocks[i * 2];
            uint64_t k2 = blocks[i * 2 + 1];

            k1 *= c1;
            k1 = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;

            h1 = rotl64(h1, 27);
            h1 += h2;
            h1 = h1 * 5 + 0x52dce729;

            k2 *= c2;
            k2 = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

            h2 = rotl64(h2, 31);
            h2 += h1;
            h2 = h2 * 5 + 0x38495ab5;
        }
        const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);

        uint64_t k1 = 0;
        uint64_t k2 = 0;

        switch (len & 15)
        {
        case 15:
            k2 ^= (uint64_t)(tail[14]) << 48;
            break;
        case 14:
            k2 ^= (uint64_t)(tail[13]) << 40;
            break;
        case 13:
            k2 ^= (uint64_t)(tail[12]) << 32;
            break;
        case 12:
            k2 ^= (uint64_t)(tail[11]) << 24;
            break;
        case 11:
            k2 ^= (uint64_t)(tail[10]) << 16;
            break;
        case 10:
            k2 ^= (uint64_t)(tail[9]) << 8;
            break;
        case 9:
            k2 ^= (uint64_t)(tail[8]) << 0;
            k2 *= c2;
            k2 = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;
            break;
        case 8:
            k1 ^= (uint64_t)(tail[7]) << 56;
            break;
        case 7:
            k1 ^= (uint64_t)(tail[6]) << 48;
            break;
        case 6:
            k1 ^= (uint64_t)(tail[5]) << 40;
            break;
        case 5:
            k1 ^= (uint64_t)(tail[4]) << 32;
            break;
        case 4:
            k1 ^= (uint64_t)(tail[3]) << 24;
            break;
        case 3:
            k1 ^= (uint64_t)(tail[2]) << 16;
            break;
        case 2:
            k1 ^= (uint64_t)(tail[1]) << 8;
            break;
        case 1:
            k1 ^= (uint64_t)(tail[0]) << 0;
            k1 *= c1;
            k1 = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
            break;
        default:  // unreachable-branch
            break;
        };
        h1 ^= len;
        h2 ^= len;

        h1 += h2;
        h2 += h1;

        h1 = fmix64(h1);
        h2 = fmix64(h2);

        h1 += h2;
        h2 += h1;
        return h1;
    }

    inline uint64_t rotl64(uint64_t x, int8_t r) const { return (x << r) | (x >> (64 - r)); }
    inline uint64_t fmix64(uint64_t k) const
    {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccd;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53;
        k ^= k >> 33;
        return k;
    }
};
}  // namespace ppc::crypto