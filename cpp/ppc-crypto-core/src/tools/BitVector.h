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
 * @file BitVector.h
 * @author: shawnhe
 * @date 2022-12-5
 */


#include "../Common.h"
#include "ppc-framework/crypto/PRNG.h"
#include <bcos-utilities/Common.h>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#pragma once

namespace ppc::crypto
{
class BitVector
{
public:
    using Ptr = std::shared_ptr<BitVector>;

    BitVector() = default;
    ~BitVector() = default;

    // init '_len' bits by '_number'
    BitVector(uint32_t _len, uint32_t _number) { m_data = boost::dynamic_bitset<>(_len, _number); }

    // init by binary string
    BitVector(const std::string& _binary) { m_data = boost::dynamic_bitset<>(_binary); }

    // init by string
    BitVector(const bcos::bytes& _data) { append(_data); }

    // get a bit by location
    bool get(uint32_t _location) { return m_data[_location]; }

    // set a bit by location
    void set(uint32_t _location, bool _bit) { m_data[_location] = _bit; }

    // generate random '_bytesLen * 8' bits
    void randomize(PRNG::Ptr _prng, uint32_t _bytesLen);

    // append from the high position
    void append(const std::string& _binary);

    // append from the high position
    void append(const bcos::bytes& _data);

    bool equals(const BitVector& _bitVector) { return m_data == _bitVector.data(); }

    boost::dynamic_bitset<>& data() { return m_data; }

    [[nodiscard]] const boost::dynamic_bitset<>& data() const { return m_data; }

    uint32_t size() { return m_data.size(); }

    void resize(uint32_t _size) { m_data.resize(_size); }

    void clear() { m_data.clear(); }

    std::string toString();

    bcos::bytes toBytes();

private:
    boost::dynamic_bitset<> m_data;
};

}  // namespace ppc::crypto
