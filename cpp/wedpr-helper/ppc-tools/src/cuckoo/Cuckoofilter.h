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
 * @file Cuckoofilter.h
 * @author: yujiechen
 * @date 2022-10-26
 */
#pragma once
#include "Common.h"
#include "HashTable.h"
#include "ppc-crypto-core/src/hash/BitMixMurmurHash.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <memory.h>

namespace ppc::tools
{
template <typename HashImpl = ppc::crypto::BitMixMurmurHash, typename HashTableImpl = HashTable>
class Cuckoofilter
{
public:
    using Ptr = std::shared_ptr<Cuckoofilter>;
    Cuckoofilter(CuckoofilterOption::Ptr _opt)
      : m_option(_opt), m_hash(std::make_shared<HashImpl>())
    {
        initCuckooFilter(_opt);
        m_hashTable = std::make_shared<HashTableImpl>(_opt);
    }

    Cuckoofilter(bcos::bytesConstRef _data)
      : m_hash(std::make_shared<HashImpl>()), m_hashTable(std::make_shared<HashTableImpl>(_data))
    {
        m_option = m_hashTable->option();
        initCuckooFilter(m_option);
    }

    virtual ~Cuckoofilter() = default;

    // serialize the cuckoo-filter(mainly serialize the hashTable)
    virtual std::string serialize() { return m_hashTable->serialize(); }

    // insert _key into the cuckoofilter
    template <typename T>
    CuckooFilterResult insert(T const& _key)
    {
        bcos::UpgradableGuard l(m_mutex);
        // the key already exists
        if (containsWithoutLock(_key))
        {
            return CuckooFilterResult::Success;
        }
        bcos::UpgradeGuard ul(l);
        return insertWithoutLock(_key);
    }

    // return the successfully inserted key-count
    template <typename T>
    bool batchInsert(T& _mutableKeyBatch)
    {
        auto startT = bcos::utcSteadyTime();
        auto orgKeySize = _mutableKeyBatch.size();
        bcos::WriteGuard l(m_mutex);
        for (auto it = _mutableKeyBatch.begin(); it != _mutableKeyBatch.end();)
        {
            // the key already exist
            if (containsWithoutLock(*it))
            {
                it = _mutableKeyBatch.erase(it);
                continue;
            }
            auto ret = insertWithoutLock(*it);
            if (ret == CuckooFilterResult::Success)
            {
                // erase the inserted element from the _keyBatch
                it = _mutableKeyBatch.erase(it);
                continue;
            }
            // not overload, but conflict, just ignore this element
            if (ret == CuckooFilterResult::OverMaxKickOff)
            {
                it++;
                continue;
            }
            // overload: break
            break;
        }
        CUCKOO_LOG(INFO) << LOG_DESC("batchInsert") << LOG_KV("inputSize", orgKeySize)
                         << LOG_KV("successSize", (orgKeySize - _mutableKeyBatch.size()))
                         << LOG_KV("left", _mutableKeyBatch.size()) << LOG_KV("load", loadFactor())
                         << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
        return _mutableKeyBatch.empty();
    }

    // check the _key exists in the cuckoofilter or not
    template <typename T>
    bool contains(T const& _key) const
    {
        bcos::ReadGuard l(m_mutex);
        return containsWithoutLock(_key);
    }

    template <typename T>
    std::vector<bool> batchContain(std::vector<T> const& _batchKey) const
    {
        bcos::ReadGuard l(m_mutex);
        std::vector<bool> result(_batchKey.size());
        for (uint64_t i = 0; i < _batchKey.size(); i++)
        {
            result[i] = containsWithoutLock(_batchKey.at(i));
        }
        return result;
    }

    // return the intersection element into _result
    template <typename T>
    void batchIntersection(std::vector<T>& _result, std::vector<T> const& _batchKey) const
    {
        bcos::ReadGuard l(m_mutex);
        for (uint64_t i = 0; i < _batchKey.size(); i++)
        {
            if (containsWithoutLock(_batchKey.at(i)))
            {
                _result.emplace_back(_batchKey.at(i));
            }
        }
    }

    template <typename T>
    std::vector<uint64_t> batchIntersectionAndGetPos(std::vector<T> const& _batchKey) const
    {
        bcos::ReadGuard l(m_mutex);
        std::vector<uint64_t> pos;
        for (uint64_t i = 0; i < _batchKey.size(); i++)
        {
            if (containsWithoutLock(_batchKey.at(i)))
            {
                pos.emplace_back(i);
            }
        }
        return pos;
    }

    // delete the _key from the cuckoofilter
    template <typename T>
    bool deleteKey(T const& _key)
    {
        bcos::WriteGuard l(m_mutex);
        return deleteKeyWithoutLock(_key);
    }

    template <typename T>
    std::vector<bool> batchDeleteKey(std::vector<T> const& _keyBatch)
    {
        bcos::WriteGuard l(m_mutex);
        std::vector<bool> result;
        for (auto const& key : _keyBatch)
        {
            result.emplace_back(deleteKeyWithoutLock(key));
        }
        return result;
    }

    // delete the key from the cuckoo-filter, return the delete-failed key into _keyBatch
    template <typename T>
    void batchDeleteAndGetRemainKeys(std::vector<T>& _keyBatch)
    {
        bcos::WriteGuard l(m_mutex);
        for (auto it = _keyBatch.begin(); it != _keyBatch.end();)
        {
            if (!deleteKeyWithoutLock(*it))
            {
                it++;
                continue;
            }
            it = _keyBatch.erase(it);
        }
    }

    double loadFactor() { return 1.0 * m_hashTable->occupiedCount() / m_totalTagSize; }
    int64_t occupiedCount() const { return m_hashTable->occupiedCount(); }

    uint64_t capacity() const { return m_hashTable->capacity(); }
    bool full() const { return m_hashTable->full(); }

protected:
    // insert the _key into cuckoo-filter
    template <typename T>
    CuckooFilterResult insertWithoutLock(T const& _key)
    {
        // the cuckoo-filter is over the load-threshold
        if (m_hashTable->full())
        {
            return CuckooFilterResult::OverLoaded;
        }
        auto fingerPrintInfo = calculateIndexAndFingerPrint(_key);
        uint64_t oldFingerPrint = 0;
        // insert the _key-finger-print into the i1 success
        if (m_hashTable->insert(
                std::get<0>(fingerPrintInfo), std::get<2>(fingerPrintInfo), oldFingerPrint, false))
        {
            return CuckooFilterResult::Success;
        }
        // insert the _key-finger-print into the i2 success
        if (m_hashTable->insert(
                std::get<1>(fingerPrintInfo), std::get<2>(fingerPrintInfo), oldFingerPrint, false))
        {
            return CuckooFilterResult::Success;
        }
        // insert the occupied index
        uint64_t index = std::get<0>(fingerPrintInfo);
        uint64_t fingerPrint = std::get<2>(fingerPrintInfo);
        for (uint16_t i = 0; i < m_option->maxKickOutCount; i++)
        {
            // replace the i2 fingerprint
            m_hashTable->insert(index, fingerPrint, oldFingerPrint, true);
            // find-out a empty position for the old-finger-print
            if (oldFingerPrint != 0)
            {
                // find the place to hold oldFingerPrint
                index = index ^ (m_hash->hash(oldFingerPrint, m_hashBitsLen) &
                                    (m_option->bucketCapacity() - 1));
                fingerPrint = oldFingerPrint;
                // reset oldFingerPrint
                oldFingerPrint = 0;
                continue;
            }
            return CuckooFilterResult::Success;
        }
        // insert into the trash bucket
        m_hashTable->insertIntoTrash(fingerPrint);
        return CuckooFilterResult::OverMaxKickOff;
    }

    // check the cuckoo-filter contains _key or not
    template <typename T>
    bool containsWithoutLock(T const& _key) const
    {
        auto fingerPrintInfo = calculateIndexAndFingerPrint(_key);
        return m_hashTable->contains(std::get<0>(fingerPrintInfo), std::get<1>(fingerPrintInfo),
            std::get<2>(fingerPrintInfo));
    }

    // delete the given _key from the cuckoo-filter
    template <typename T>
    bool deleteKeyWithoutLock(T const& _key)
    {
        auto fingerprintInfo = calculateIndexAndFingerPrint(_key);
        if (m_hashTable->deleteFingerPrint(
                std::get<0>(fingerprintInfo), std::get<2>(fingerprintInfo)))
        {
            return true;
        }
        if (m_hashTable->deleteFingerPrint(
                std::get<1>(fingerprintInfo), std::get<2>(fingerprintInfo)))
        {
            return true;
        }
        // delete from the trashBucket
        if (m_hashTable->deleteFromTrashBucket(std::get<2>(fingerprintInfo)))
        {
            return true;
        }
        return false;
    }

private:
    void initCuckooFilter(CuckoofilterOption::Ptr _opt)
    {
        m_option = _opt;
        if (m_option->tagBits % 8 != 0)
        {
            BOOST_THROW_EXCEPTION(InvalidCuckooFilterOption()
                                  << bcos::errinfo_comment("the tags size must be multiple of 8"));
        }
        if (m_option->capacity == 0)
        {
            BOOST_THROW_EXCEPTION(InvalidCuckooFilterOption()
                                  << bcos::errinfo_comment("the capacity must be larger than 0"));
        }
        if (m_option->tagBits > 64)
        {
            BOOST_THROW_EXCEPTION(InvalidCuckooFilterOption() << bcos::errinfo_comment(
                                      "the tagBits must be no larger than 64"));
        }
        m_hashBitsLen = (m_option->tagBits <= 32 ? 32 : 64);
        m_tagBytes = m_option->tagBits >> 3;
        // update the capacity to 2^n(the cuckoo-filter requires the capacity to be 2^n)
        m_option->calculateBucketCapacity();
        m_totalTagSize = (uint64_t)m_option->bucketCapacity() * (uint64_t)m_option->tagNumPerBucket;
        CUCKOO_LOG(INFO) << printCuckooFilterOption(_opt);
    }

    uint64_t calculateFingerPrint(uint64_t _hash) const
    {
        auto fingerPrint = (uint64_t)(_hash >> (m_hashBitsLen - m_option->tagBits));
        // the case finger is 0
        if (0 == fingerPrint)
        {
            return 1;
        }
        return fingerPrint;
    }

    template <typename T>
    std::tuple<uint64_t, uint64_t, uint64_t> calculateIndexAndFingerPrint(T const _key) const
    {
        uint64_t hashResult = m_hash->hash(_key, m_hashBitsLen);
        // calculate i1
        uint64_t i1 = hashResult & (m_option->bucketCapacity() - 1);
        // calculate fingerprint
        uint64_t fingerPrint = calculateFingerPrint(hashResult);
        // calculate i2
        uint64_t i2 =
            i1 ^ (m_hash->hash(fingerPrint, m_hashBitsLen) & (m_option->bucketCapacity() - 1));
        return std::make_tuple(i1, i2, fingerPrint);
    }

private:
    CuckoofilterOption::Ptr m_option;
    std::shared_ptr<HashImpl> m_hash;
    std::shared_ptr<HashTableImpl> m_hashTable;
    uint8_t m_tagBytes;
    uint8_t m_hashBitsLen;
    uint64_t m_totalTagSize = 0;
    mutable bcos::SharedMutex m_mutex;
};
}  // namespace ppc::tools
