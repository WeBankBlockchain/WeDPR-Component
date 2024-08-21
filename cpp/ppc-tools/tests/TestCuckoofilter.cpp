/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file TestCuckoofilter.cpp
 * @author: yujiechen
 * @date 2022-10-26
 */
#include "ppc-crypto-core/src/hash/SM3Hash.h"
#include "ppc-crypto-core/src/hash/Sha512Hash.h"
#include "ppc-tools/src/cuckoo/Cuckoofilter.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
#include <random>

using namespace ppc::tools;
using namespace ppc::crypto;
using namespace bcos;

using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(CuckoofilterTest, TestPromptFixture)

void testCuckoofilterFunc(
    std::shared_ptr<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>> filter,
    uint64_t keySize)
{
    // check empty key
    for (uint64_t i = 1; i <= keySize; i++)
    {
        BOOST_CHECK(filter->contains<uint64_t>(i) == false);
    }
    // insert keys
    for (uint64_t i = 1; i <= keySize; i++)
    {
        filter->insert<uint64_t>(i);
        BOOST_CHECK(filter->contains<uint64_t>(i) == true);
    }
    // check the occupied-count
    BOOST_CHECK((uint64_t)filter->occupiedCount() == keySize);
    BOOST_CHECK(filter->loadFactor() < 1);

    // test encode/decode
    auto encodedData = filter->serialize();
    auto decodedFilter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(
        bcos::bytesConstRef((const unsigned char*)encodedData.data(), encodedData.size()));
    // check the keys
    for (uint64_t i = 1; i <= keySize; i++)
    {
        BOOST_CHECK(decodedFilter->contains<int64_t>(i) == true);
    }
    BOOST_CHECK(decodedFilter->contains<int64_t>(keySize + 1) == false);
}

void randomNumberTest(CuckoofilterOption::Ptr option, uint64_t keySize)
{
    uint64_t maxValue = 1000000000;
    auto filter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(option);

    std::set<uint64_t> randomValuesSet;
    std::mt19937_64 rng(bcos::utcSteadyTime());
    std::uniform_int_distribution<uint64_t> dis(1, maxValue);
    for (uint64_t i = 0; i < keySize; i++)
    {
        auto value = dis(rng);
        randomValuesSet.insert(value);
    }
    std::vector<uint64_t> randomValues(randomValuesSet.begin(), randomValuesSet.end());
    // insert and check randomValues
    std::set<uint64_t> tmpRandomValues(randomValues.begin(), randomValues.end());
    auto result = filter->batchInsert(tmpRandomValues);
    BOOST_CHECK(result == true);
    BOOST_CHECK(tmpRandomValues.empty());
    for (uint64_t i = 0; i < randomValues.size(); i++)
    {
        BOOST_CHECK(filter->contains<uint64_t>(randomValues.at(i)) == true);
    }
    // batchContain
    auto batchContainResult = filter->batchContain<uint64_t>(randomValues);
    BOOST_CHECK(batchContainResult.size() == randomValues.size());
    for (uint64_t i = 0; i < randomValues.size(); i++)
    {
        BOOST_CHECK(batchContainResult.at(i) == true);
    }
    // test encode/decode
    auto encodedData = filter->serialize();
    auto decodedFilter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(
        bcos::bytesConstRef((const unsigned char*)encodedData.data(), encodedData.size()));
    // check the keys
    for (uint64_t i = 0; i < randomValues.size(); i++)
    {
        BOOST_CHECK(decodedFilter->contains<uint64_t>(randomValues.at(i)) == true);
    }
    // delete and check
    for (uint64_t i = 0; i < randomValues.size() / 2; i++)
    {
        BOOST_CHECK(filter->deleteKey<uint64_t>(randomValues.at(i)) == true);
    }
    uint64_t falsePositiveCount = 0;
    for (uint64_t i = 0; i < randomValues.size() / 2; i++)
    {
        if (filter->contains<uint64_t>(randomValues.at(i)) == true)
        {
            falsePositiveCount++;
        }
    }
    std::cout << "falsePositiveCount: " << falsePositiveCount << std::endl;
    // batchDelete
    filter->batchDeleteKey<uint64_t>(randomValues);
    for (uint64_t i = 0; i < randomValues.size(); i++)
    {
        BOOST_CHECK(filter->contains<uint64_t>(randomValues.at(i)) == false);
    }
}

void bytesKeyTest(Hash::Ptr hashImpl, CuckoofilterOption::Ptr option, uint64_t keySize)
{
    auto filter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(option);
    std::vector<bcos::bytes> hashList;
    for (uint64_t i = 0; i < keySize; i++)
    {
        std::string input = std::to_string(i);
        bcos::bytes inputData(input.begin(), input.end());
        hashList.emplace_back(hashImpl->hash(ref(inputData)));
    }
    // insert the hashList and check
    std::set<bcos::bytes> tmpHashList(hashList.begin(), hashList.end());
    auto result = filter->batchInsert(tmpHashList);
    BOOST_CHECK(tmpHashList.empty() == true);
    BOOST_CHECK(result == true);
    // insert with std::string_view and std::string(insert multiple-times)
    for (uint64_t i = 0; i < hashList.size(); i++)
    {
        std::string dataStr = std::string(hashList.at(i).begin(), hashList.at(i).end());
        filter->insert<std::string>(dataStr);
        filter->insert<std::string_view>(std::string_view(dataStr));
    }
    for (uint64_t i = 0; i < hashList.size(); i++)
    {
        BOOST_CHECK(filter->contains<bcos::bytes>(hashList.at(i)) == true);
    }
    // batchContain
    auto batchContainResult = filter->batchContain<bcos::bytes>(hashList);
    BOOST_CHECK(batchContainResult.size() == hashList.size());
    for (uint64_t i = 0; i < hashList.size(); i++)
    {
        BOOST_CHECK(batchContainResult.at(i) == true);
    }
    // test encode/decode
    auto encodedData = filter->serialize();
    auto decodedFilter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(
        bcos::bytesConstRef((const unsigned char*)encodedData.data(), encodedData.size()));
    // check the keys
    for (uint64_t i = 0; i < hashList.size(); i++)
    {
        BOOST_CHECK(decodedFilter->contains<bcos::bytes>(hashList.at(i)) == true);
    }
    // delete and check
    for (uint64_t i = 0; i < hashList.size() / 2; i++)
    {
        BOOST_CHECK(filter->deleteKey<bcos::bytes>(hashList.at(i)) == true);
    }
    uint64_t falsePositiveCount = 0;
    for (uint64_t i = 0; i < hashList.size() / 2; i++)
    {
        if (filter->contains<bcos::bytes>(hashList.at(i)) == true)
        {
            falsePositiveCount++;
        }
    }
    std::cout << "bytesKeyTest falsePositiveCount: " << falsePositiveCount << std::endl;
    // batchDelete
    filter->batchDeleteKey<bcos::bytes>(hashList);
    for (uint64_t i = 0; i < hashList.size(); i++)
    {
        BOOST_CHECK(filter->contains<bcos::bytes>(hashList.at(i)) == false);
    }
}

BOOST_AUTO_TEST_CASE(testCuckoofilter)
{
    auto sm3Hash = std::make_shared<SM3Hash>();
    auto sha512Hash = std::make_shared<Sha512Hash>();
    // option with tagBits: 16, tagNumPerBucket: 4, maxKickOutCount: 20
    auto option = std::make_shared<CuckoofilterOption>();
    option->capacity = 10000;
    auto filter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(option);
    uint64_t keySize = 5000;
    testCuckoofilterFunc(filter, keySize);
    randomNumberTest(option, keySize);
    bytesKeyTest(sm3Hash, option, keySize);
    bytesKeyTest(sha512Hash, option, keySize);

    // option with tagBits: 32, tagNumPerBucket: 4, maxKickOutCount: 10
    option = std::make_shared<CuckoofilterOption>();
    option->tagBits = 32;
    option->maxKickOutCount = 10;
    option->capacity = 20000;
    filter = std::make_shared<Cuckoofilter<ppc::crypto::BitMixMurmurHash, HashTable>>(option);
    keySize = 10000;
    testCuckoofilterFunc(filter, keySize);
    randomNumberTest(option, keySize);
    bytesKeyTest(sm3Hash, option, keySize);
    bytesKeyTest(sha512Hash, option, keySize);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test