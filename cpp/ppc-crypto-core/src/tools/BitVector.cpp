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
 * @file BitVector.cpp
 * @author: shawnhe
 * @date 2022-12-5
 */

#include "BitVector.h"
#include <bitset>

using namespace ppc::crypto;

void BitVector::randomize(PRNG::Ptr _prng, uint32_t _bytesLen)
{
    m_data.clear();
    auto randomBytes = _prng->generate(_bytesLen);
    append(randomBytes);
}

void BitVector::append(const bcos::bytes& _data)
{
    int start = _data.size() - 1;
    for (int i = start; i >= 0; --i)
    {
        auto bits = std::bitset<8>(_data[i]);
        for (int j = 0; j < 8; ++j)
        {
            m_data.push_back(bits[j]);
        }
    }
}

void BitVector::append(const std::string& _binary)
{
    auto newBits = boost::dynamic_bitset<>(_binary);
    for (uint32_t i = 0; i < newBits.size(); ++i)
    {
        m_data.push_back(newBits[i]);
    }
}

std::string BitVector::toString()
{
    std::string result;
    boost::to_string(m_data, result);
    return result;
}

bcos::bytes BitVector::toBytes()
{
    if (m_data.size() % 8)
    {
        BOOST_THROW_EXCEPTION(
            BitVectorException() << bcos::errinfo_comment("the size must be divisible by 8"));
    }

    uint32_t len = m_data.size() / 8;
    bcos::bytes result(len);

    for (uint32_t i = 0; i < len; ++i)
    {
        bcos::byte b = 0;
        b |= m_data[i * 8 + 0] << 0;
        b |= m_data[i * 8 + 1] << 1;
        b |= m_data[i * 8 + 2] << 2;
        b |= m_data[i * 8 + 3] << 3;
        b |= m_data[i * 8 + 4] << 4;
        b |= m_data[i * 8 + 5] << 5;
        b |= m_data[i * 8 + 6] << 6;
        b |= m_data[i * 8 + 7] << 7;
        result[len - 1 - i] = b;
    }

    return result;
}