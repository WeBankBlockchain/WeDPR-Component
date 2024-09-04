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
 * @file TestOprf.cpp
 * @author: shawnhe
 * @date 2022-11-3
 */

#include "ppc-crypto-core/src/hash/HashFactoryImpl.h"
#include "ppc-crypto/src/ecc/EccCryptoFactoryImpl.h"
#include <bcos-utilities/DataConvertUtility.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <ppc-crypto/src/oprf/EcdhOprf.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(EcdhOprfTest, TestPromptFixture)

void runEcdhOprf(EccCrypto::Ptr _clientEcc, EccCrypto::Ptr _serverEcc, Hash::Ptr _clientHash,
    Hash::Ptr _serverHash)
{
    uint16_t outputSize = 32;
    auto client = std::make_shared<EcdhOprfClient>(outputSize, _clientHash, _clientEcc);
    auto server = std::make_shared<EcdhOprfServer>(outputSize, _serverHash, _serverEcc);

    // test one item
    std::string inputStr = "wer23dfddd4lkejskdf";
    bcos::bytes inputData(inputStr.begin(), inputStr.end());

    auto blindedItem = client->blind(inputStr);
    auto evaluatedItem = server->evaluate(blindedItem);
    auto outputC = client->finalize(inputStr, evaluatedItem);
    auto outputS = server->fullEvaluate(inputStr);

    std::cout << "#### oprf client output: " << *toHexString(outputC) << std::endl;
    std::cout << "#### oprf server output: " << *toHexString(outputS) << std::endl;
    BOOST_CHECK(outputC == outputS);

    // test batch
    std::vector<std::string> inputs;
    for (uint64_t i = 0; i < 1024; i++)
    {
        inputs.emplace_back(inputStr + std::to_string(i));
    }

    auto blindedItems = client->blind(inputs);
    auto evaluatedItems = server->evaluate(blindedItems);
    auto outputCs = client->finalize(inputs, evaluatedItems);
    auto outputSs = server->fullEvaluate(inputs);

    for (uint64_t i = 0; i < 1024; i++)
    {
        BOOST_CHECK(outputCs[i] == outputSs[i]);
    }
}


BOOST_AUTO_TEST_CASE(testEcdhOprf)
{
    auto eccFactory = std::make_shared<EccCryptoFactoryImpl>();
    auto hashFactory = std::make_shared<HashFactoryImpl>();

    // test sm
    auto sm3Hash = hashFactory->createHashImpl((int8_t)ppc::protocol::HashImplName::SM3);
    auto sm2Ecc0 = eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::SM2, sm3Hash);
    auto sm2Ecc1 = eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::SM2, sm3Hash);
    runEcdhOprf(sm2Ecc0, sm2Ecc1, sm3Hash, sm3Hash);

    // test others
    std::vector<Hash::Ptr> hashSet = {
        hashFactory->createHashImpl((int8_t)ppc::protocol::HashImplName::SHA256),
        hashFactory->createHashImpl((int8_t)ppc::protocol::HashImplName::SHA512),
        hashFactory->createHashImpl((int8_t)ppc::protocol::HashImplName::BLAKE2b)};

    for (auto& hashImpl : hashSet)
    {
        std::vector<EccCrypto::Ptr> eccSet0 = {
            eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::ED25519, hashImpl),
            eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::SECP256K1, hashImpl),
            eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::P256, hashImpl)};
        std::vector<EccCrypto::Ptr> eccSet1 = {
            eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::ED25519, hashImpl),
            eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::SECP256K1, hashImpl),
            eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::P256, hashImpl)};
        for (uint64_t i = 0; i < eccSet0.size(); i++)
        {
            runEcdhOprf(eccSet0[i], eccSet1[i], hashImpl, hashImpl);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
