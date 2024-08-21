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
 * @file HashTable.cpp
 * @author: yujiechen
 * @date 2022-10-26
 */
#include "HashTable.h"

using namespace ppc::tools;

std::string HashTable::serialize()
{
    std::string encodedData;
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string>> outputStream(
        encodedData);
    boost::archive::binary_oarchive archive(outputStream,
        boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_tracking);
    archive << m_option->capacity;
    archive << m_option->tagBits;
    archive << m_option->tagNumPerBucket;
    archive << m_option->maxKickOutCount;
    archive << m_buckets;
    // serialize the trash-bucket
    archive << m_trashBucket;
    outputStream.flush();
    return encodedData;
}

void HashTable::deserialize(bcos::bytesConstRef _data)
{
    boost::iostreams::stream<boost::iostreams::array_source> inputStream(
        (const char*)_data.data(), _data.size());
    boost::archive::binary_iarchive archive(inputStream,
        boost::archive::no_header | boost::archive::no_codecvt | boost::archive::no_tracking);
    archive >> m_option->capacity;
    archive >> m_option->tagBits;
    archive >> m_option->tagNumPerBucket;
    archive >> m_option->maxKickOutCount;
    archive >> m_buckets;
    // deserialize the trash-bucket
    archive >> m_trashBucket;
}

uint64_t HashTable::getFingerPrint(const uint64_t _entryIndex, const uint32_t _bucketIndex) const
{
    // empty finger-print
    auto it = m_buckets.find(_entryIndex);
    if (it == m_buckets.end())
    {
        return 0;
    }
    auto const& bucket = it->second;
    // empty bucket
    if (bucket.size() == 0)
    {
        return 0;
    }
    // Note: use little-endian here
    // get the bytes-data
    int64_t fingerPrint = 0;
    auto offset = _bucketIndex * m_bytesPerTag;
    for (uint32_t i = offset; i < (offset + m_bytesPerTag); i++)
    {
        fingerPrint <<= 8;
        fingerPrint |= bucket.at(i);
    }
    return fingerPrint;
}

bool HashTable::insert(
    const uint64_t _pos, const uint64_t _fingerPrint, uint64_t& _oldFingerPrint, bool _kickOut)
{
    // find the un-occupied bucket
    for (uint32_t bucketIndex = 0; bucketIndex < m_option->tagNumPerBucket; bucketIndex++)
    {
        if (getFingerPrint(_pos, bucketIndex) == 0)
        {
            writeFingerPrint(_pos, bucketIndex, _fingerPrint);
            m_occupiedCount++;
            return true;
        }
    }
    // random choose one fingerprint from the bucket to kickOut
    if (_kickOut)
    {
        srand(bcos::utcSteadyTime());
        auto selectedBucket = rand() % (m_option->tagNumPerBucket);
        _oldFingerPrint = getFingerPrint(_pos, selectedBucket);
        writeFingerPrint(_pos, selectedBucket, _fingerPrint);
        return true;
    }
    return false;
}

bool HashTable::deleteFingerPrint(const uint64_t _pos, const uint64_t _fingerPrint)
{
    for (uint32_t bucketIndex = 0; bucketIndex < m_option->tagNumPerBucket; bucketIndex++)
    {
        if (getFingerPrint(_pos, bucketIndex) == _fingerPrint)
        {
            writeFingerPrint(_pos, bucketIndex, 0);
            m_occupiedCount.store(std::max<int64_t>(0, m_occupiedCount.load() - 1));
            return true;
        }
    }
    return false;
}

void HashTable::writeFingerPrint(
    const uint64_t _entryIndex, const uint32_t _bucketIndex, uint64_t const _fingerPrint)
{
    auto it = m_buckets.find(_entryIndex);
    if (it == m_buckets.end())
    {
        m_buckets[_entryIndex] = Bucket();
    }
    auto& bucket = m_buckets[_entryIndex];
    if (bucket.size() <= m_bytesPerBucket)
    {
        bucket.resize(m_bytesPerBucket);
        m_capacity += m_bytesPerBucket;
    }
    // write the fingerprint into the bucket
    auto offset = _bucketIndex * m_bytesPerTag;
    auto endPos = offset + (m_bytesPerTag - 1);
    auto fingerPrintData = _fingerPrint;
    for (int64_t i = endPos; i >= offset; i--)
    {
        bucket[i] = fingerPrintData & 0xff;
        fingerPrintData >>= 8;
    }
}

bool HashTable::contains(const uint64_t _i1, const uint64_t _i2, const uint64_t _fingerPrint) const
{
    for (uint32_t bucketIndex = 0; bucketIndex < m_option->tagNumPerBucket; bucketIndex++)
    {
        if ((getFingerPrint(_i1, bucketIndex) == _fingerPrint) ||
            (getFingerPrint(_i2, bucketIndex) == _fingerPrint))
        {
            return true;
        }
    }
    // check the trash-bucket: hit in the trash bucket
    if (m_trashBucket.count(_fingerPrint))
    {
        return true;
    }
    return false;
}