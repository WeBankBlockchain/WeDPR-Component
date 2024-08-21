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
 * @file BLAKE2bPRNG.cpp
 * @author: shawnhe
 * @date 2022-12-4
 */

#include "BLAKE2bPRNG.h"
#include <tbb/parallel_for.h>

namespace ppc::crypto
{

// generate `_bytesLen` bytes random numbers
void BLAKE2bPRNG::generate(bcos::byte* _dest, uint64_t _bytesLen)
{
    uint64_t bytesIdx = m_totalOutputs % BUFFER_CAPACITY;

    uint64_t step = std::min(_bytesLen, BUFFER_CAPACITY - bytesIdx);
    memcpy(_dest, m_buffer.data() + bytesIdx, step);
    _dest += step;
    m_totalOutputs += step;

    _bytesLen -= step;
    uint64_t round = _bytesLen / BUFFER_CAPACITY;
    uint64_t remain = _bytesLen % BUFFER_CAPACITY;

    if (round)
    {
        parallelGen(_dest, m_totalOutputs / BUFFER_CAPACITY, round);
        _dest += round * BUFFER_CAPACITY;
        m_totalOutputs += round * BUFFER_CAPACITY;
    }

    uint64_t currentIndex = m_totalOutputs / BUFFER_CAPACITY;
    bcos::bytes indexData(
        (bcos::byte*)(&currentIndex), (bcos::byte*)(&currentIndex) + sizeof(uint64_t));
    m_buffer = m_blake2Hash->hash(bcos::ref(indexData), bcos::ref(m_seeds), BUFFER_CAPACITY);

    if (remain)
    {
        memcpy(_dest, m_buffer.data(), remain);
        m_totalOutputs += remain;
    }
}

void BLAKE2bPRNG::parallelGen(bcos::byte* _dest, uint64_t _currentRound, uint64_t _remainRound)
{
    tbb::parallel_for(tbb::blocked_range<uint64_t>(0U, _remainRound), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            uint64_t currentIndex = _currentRound + i;
            bcos::bytes indexData(
                (bcos::byte*)(&currentIndex), (bcos::byte*)(&currentIndex) + sizeof(uint64_t));
            auto temp = m_blake2Hash->hash(bcos::ref(indexData), bcos::ref(m_seeds), BUFFER_CAPACITY);
            memcpy(_dest + i * BUFFER_CAPACITY, temp.data(), BUFFER_CAPACITY);
        }
    });
}

}  // namespace ppc::crypto
