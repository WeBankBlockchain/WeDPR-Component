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
 * @file TestHash.cpp
 * @author: yujiechen
 * @date 2022-11-2
 */
#include "ppc-crypto-core/src/hash/BLAKE2bHash.h"
#include "ppc-crypto-core/src/hash/MD5Hash.h"
#include "ppc-crypto-core/src/hash/SM3Hash.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto-core/src/hash/Sha512Hash.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(hashTest, TestPromptFixture)

void testHash(Hash::Ptr _hashImpl, uint64_t expectedHashLen)
{
    std::string input = "abcdwerwlerkwel" + std::to_string(utcSteadyTime());
    bcos::bytes inputData(input.begin(), input.end());
    // check hash
    auto hashResult = _hashImpl->hash(ref(inputData));
    for (int i = 0; i < 1000; i++)
    {
        BOOST_CHECK(_hashImpl->hash(ref(inputData)) == hashResult);
        BOOST_CHECK(hashResult.size() == expectedHashLen);
    }

    // check update
    auto state = _hashImpl->init();
    _hashImpl->update(state, ref(inputData));
    auto finalHashResult = _hashImpl->final(state);
    BOOST_CHECK(finalHashResult == hashResult);

    // check update multiple input
    std::string input2 = "werwelr234weweskdsj2342";
    bcos::bytes inputData2(input2.begin(), input2.end());
    state = _hashImpl->init();
    _hashImpl->update(state, ref(inputData));
    _hashImpl->update(state, ref(inputData2));
    finalHashResult = _hashImpl->final(state);
    for (int i = 0; i < 1000; i++)
    {
        state = _hashImpl->init();
        _hashImpl->update(state, ref(inputData));
        _hashImpl->update(state, ref(inputData2));
        BOOST_CHECK(_hashImpl->final(state) == finalHashResult);
    }
}

void checkFixedInput(
    Hash::Ptr _hashImpl, std::string const& _input, std::string const& _expectedHash)
{
    bytes inputData(_input.begin(), _input.end());
    auto result = _hashImpl->hash(ref(inputData));
    std::cout << *toHexString(result) << std::endl;
    BOOST_CHECK(*toHexString(result) == _expectedHash);
}
BOOST_AUTO_TEST_CASE(testSha256Hash)
{
    testHash(std::make_shared<Sha256Hash>(), 32);
    std::string input = "abcdwerwlerkwel";
    std::string expectedSha256 = "d7a5cc8e0f256e5c3f7f1c551d9206a22983da73969eb54b40b111c378ea4b70";
    checkFixedInput(std::make_shared<Sha256Hash>(), input, expectedSha256);
}

BOOST_AUTO_TEST_CASE(testSha512Hash)
{
    testHash(std::make_shared<Sha512Hash>(), 64);
    std::string input = "abcdwerwlerkwel";
    std::string expectedHash =
        "f8e36730b763de5699d111fb65aae0e73afaedb914f634ad8210891f4b5b88f8ffc3fcc557c4273f61cc5177a4"
        "67481a8d7b91a86a4c2de9ce342e91e7c3144d";
    checkFixedInput(std::make_shared<Sha512Hash>(), input, expectedHash);
}

BOOST_AUTO_TEST_CASE(testSM3Hash)
{
    testHash(std::make_shared<SM3Hash>(), 32);
    std::string input = "abcdwerwlerkwel";
    std::string expectedHash = "9e057883cc739dc1ee95ae65a0e86b946330c77e48123e9cea2b31a43cb69db1";
    checkFixedInput(std::make_shared<SM3Hash>(), input, expectedHash);
}

BOOST_AUTO_TEST_CASE(testMD5Hash)
{
    testHash(std::make_shared<MD5Hash>(), 16);
    std::string input = "abcdwerwlerkwel";
    std::string expectedHash = "a3eeda2c6b29a2518d0945baf3cfd220";
    checkFixedInput(std::make_shared<MD5Hash>(), input, expectedHash);
}

BOOST_AUTO_TEST_CASE(testBLAKE2bHash)
{
    testHash(std::make_shared<BLAKE2bHash>(), 64);

    std::string input = "abcdwerwlerkwel";
    std::string expected512Hash =
        "fa693cb55a305997ed33839bf540d88fdcd7f343e6ff4aa34d31fbdfaf6567569e8dd46a535e7d44b77de5e784"
        "d8ac2e1a6413b473be3790c9f407594f1ead06";
    checkFixedInput(std::make_shared<BLAKE2bHash>(), input, expected512Hash);


    bytes inputData(input.begin(), input.end());
    auto hashImpl = std::make_shared<BLAKE2bHash>();
    auto result = hashImpl->hash(ref(inputData), 32);
    std::string expected256Hash =
        "869cf8d5848f78a3a9c076e5d36a0a05ef0f37c2837a952b6590981762b3c9b3";
    BOOST_CHECK(*toHexString(result) == expected256Hash);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test