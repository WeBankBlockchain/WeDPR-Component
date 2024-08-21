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
 * @file HashTable.h
 * @author: yujiechen
 * @date 2022-10-26
 */
#pragma once
#include "Common.h"
#include <boost/archive/basic_archive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
namespace ppc::tools
{
class HashTable
{
public:
    using Ptr = std::shared_ptr<HashTable>;
    HashTable(CuckoofilterOption::Ptr _option) : m_option(_option) { initHashTableParams(); }

    HashTable(bcos::bytesConstRef _data) : m_option(std::make_shared<CuckoofilterOption>())
    {
        m_capacity = _data.size();
        deserialize(_data);
        initHashTableParams();
    }

    virtual ~HashTable() = default;
    /**
     * @brief check the hashTable contains given finger-print in pos _i1 or _i2
     *
     * @param _i1 the pos i1
     * @param _i2 the pos i2
     * @param _fingerPrint the finger-print
     * @return true find the finger-print
     * @return false the finger-print doesn't exist in the HashTable
     */
    bool contains(const uint64_t _i1, const uint64_t _i2, const uint64_t _fingerPrint) const;

    /**
     * @brief try to insert finger-print into the given pos
     *
     * @param _pos the pos that required to insert the finger-print
     * @param _fingerPrint the finger-print to be inserted
     * @param _oldFingerPrint the kickOuted finger-print
     * @return true insert success
     * @return false insert failed
     */
    // Note: use lazy allocate policy here
    bool insert(const uint64_t _pos, const uint64_t _fingerPrint, uint64_t& _oldFingerPrint,
        bool _kickOut = true);

    // delete the given finger-print front the hashTable
    bool deleteFingerPrint(const uint64_t _pos, const uint64_t _fingerPrint);
    virtual std::string serialize();
    virtual void deserialize(bcos::bytesConstRef _data);
    CuckoofilterOption::Ptr option() const { return m_option; }
    int64_t occupiedCount() const { return m_occupiedCount.load(); }

    uint64_t capacity() const { return m_capacity; }

    // the hashTable is full or not
    bool full() const { return m_trashBucket.size() >= m_option->trashBucketSize; }

    bool insertIntoTrash(uint64_t _fingerPrint)
    {
        if (m_trashBucket.count(_fingerPrint))
        {
            return true;
        }
        if (m_trashBucket.size() >= m_option->trashBucketSize)
        {
            return false;
        }
        m_trashBucket.insert(_fingerPrint);
        return true;
    }

    bool deleteFromTrashBucket(uint64_t _fingerPrint)
    {
        auto it = m_trashBucket.find(_fingerPrint);
        if (it != m_trashBucket.end())
        {
            m_trashBucket.erase(it);
            return true;
        }
        return false;
    }

protected:
    void initHashTableParams()
    {
        m_bytesPerBucket = (uint32_t)(m_option->tagBits * m_option->tagNumPerBucket) >> 3;
        // Note: the tagBits of Cuckoofilter should be multiple of 8
        m_bytesPerTag = (uint32_t)(m_option->tagBits >> 3);
        // the bucket-capacity
        CUCKOO_LOG(INFO) << LOG_DESC("create HashTable")
                         << LOG_KV("tagBits", std::to_string(m_option->tagBits))
                         << LOG_KV("tagNumPerBucket", std::to_string(m_option->tagNumPerBucket))
                         << LOG_KV("bucketCapacity", m_option->bucketCapacity())
                         << LOG_KV("bytesPerBucket", m_bytesPerBucket)
                         << LOG_KV("capacity", m_option->capacity);
    }

private:
    uint64_t getFingerPrint(const uint64_t _entryIndex, const uint32_t _bucketIndex) const;
    // lazy-allocate
    void writeFingerPrint(
        const uint64_t _entryIndex, const uint32_t _bucketIndex, uint64_t const _fingerPrint);

private:
    // the option
    CuckoofilterOption::Ptr m_option;
    // bytes-per-bucket(determined by the tagBits and the tagNumPerBucket)
    uint32_t m_bytesPerBucket;
    // bytes per fingerprint
    uint32_t m_bytesPerTag;
    // Note: the unordered_map can't ensure all the node with the same cuckoo-filter encoded into
    // the same data, so we use map here
    using Bucket = bcos::bytes;
    std::map<uint32_t, Bucket> m_buckets;
    // trash bucket, storing elements kicked out
    // Note: the trash bucket is neccessary to accept the element over max-kick-out-count
    std::set<uint64_t> m_trashBucket;
    std::atomic<uint64_t> m_capacity = {0};

    // the occupied-count
    std::atomic<int64_t> m_occupiedCount = {0};
};
}  // namespace ppc::tools