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
 * @file PRNG.h
 * @author: shawnhe
 * @date 2022-11-29
 */

#pragma once
#include "../protocol/Protocol.h"
#include <bcos-utilities/Common.h>
#include <memory>

namespace ppc::crypto
{
class PRNG
{
public:
    using Ptr = std::shared_ptr<PRNG>;

    // default constructor is not allowed
    PRNG() = delete;
    virtual ~PRNG() = default;

    PRNG(const bcos::bytes& _seed) : m_seeds(_seed) {}

    virtual ppc::protocol::PRNGImplName type() const = 0;

    // generate `_bytesLen` bytes random numbers, saved in _dest
    virtual void generate(bcos::byte* _dest, uint64_t _bytesLen) = 0;
    bcos::bytes generate(uint64_t _bytesLen)
    {
        bcos::bytes res(_bytesLen);
        generate(res.data(), _bytesLen);
        return res;
    }

    // generate one T type random
    template <typename T>
    typename std::enable_if<std::is_standard_layout<T>::value && std::is_trivial<T>::value, T>::type
    generate()
    {
        T ret;
        generate((bcos::byte*)&ret, sizeof(T));
        return ret;
    }

    // generate '_size' T type randoms
    template <typename T>
    typename std::enable_if<std::is_standard_layout<T>::value && std::is_trivial<T>::value,
        void>::type
    generate(T* dest, uint64_t _size)
    {
        uint64_t totalLen = _size * sizeof(T);
        generate((bcos::byte*)dest, totalLen);
    }

    const bcos::bytes& seeds() const { return m_seeds; }
    uint64_t totalOutputs() const { return m_totalOutputs; }

protected:
    bcos::bytes m_seeds;

    // the length of the random number that has been output
    uint64_t m_totalOutputs{0};
};

}  // namespace ppc::crypto