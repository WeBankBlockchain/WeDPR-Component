/*
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
 * @file Common.h
 * @author: yujiechen
 * @date 2022-10-26
 */
#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>

#define CUCKOO_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("CUCKOO")
namespace ppc::tools
{
DERIVE_PPC_EXCEPTION(InvalidCuckooFilterOption);
inline uint64_t upperpower2(uint64_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}
struct CuckoofilterOption
{
    using Ptr = std::shared_ptr<CuckoofilterOption>;
    using ConstPtr = std::shared_ptr<CuckoofilterOption const>;
    // the capacity
    uint64_t capacity = 0;
    // the tag-size, default is 64bits
    uint8_t tagBits = 64;
    // the tagNumPerBucket, default is 4
    uint8_t tagNumPerBucket = 4;
    // The maximum number of times an element has been kicked
    uint16_t maxKickOutCount = 20;
    // the trash bucket size
    uint64_t trashBucketSize = 10000;

    // calculate the bucketCapacity
    void calculateBucketCapacity()
    {
        m_bucketCapacity = upperpower2(std::max<uint64_t>(1, capacity / tagNumPerBucket));
    }
    uint64_t bucketCapacity() const { return m_bucketCapacity; }

private:
    uint64_t m_bucketCapacity = 0;
};

enum CuckooFilterResult : int
{
    OverLoaded,
    OverMaxKickOff,
    Success,
};

inline std::string printCuckooFilterOption(CuckoofilterOption::ConstPtr _option)
{
    std::ostringstream stringstream;
    stringstream << LOG_KV("capacity", _option->capacity)
                 << LOG_KV("tagBits", (int)_option->tagBits)
                 << LOG_KV("buckets", (int)_option->tagNumPerBucket)
                 << LOG_KV("maxKickOutCount", _option->maxKickOutCount)
                 << LOG_KV("trashBucketSize", _option->trashBucketSize);
    return stringstream.str();
}

}  // namespace ppc::tools