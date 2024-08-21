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
 * @file TestRandomot.cpp
 * @author: shawnhe
 * @date 2022-12-6
 */


#include "ppc-crypto-core/src/hash/HashFactoryImpl.h"
#include "ppc-crypto-core/src/tools/BitVector.h"
#include "ppc-crypto/src/ecc/EccCryptoFactoryImpl.h"
#include "ppc-crypto/src/prng/AESPRNG.h"
#include "ppc-crypto/src/randomot/SimplestOT.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::crypto;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(RandomotTest, TestPromptFixture)

void runRandomOT(EccCrypto::Ptr _clientEcc, EccCrypto::Ptr _serverEcc, Hash::Ptr _clientHash,
    Hash::Ptr _serverHash)
{
    auto simplestOt0 = std::make_shared<SimplestOT>(_clientEcc, _clientHash);
    auto simplestOt1 = std::make_shared<SimplestOT>(_serverEcc, _serverHash);

    auto chooses = std::make_shared<BitVector>();
    bcos::bytes seed{'b', '1', 'c', '0'};
    auto prng = std::make_shared<AESPRNG>(seed);

    uint32_t otNumber = 1024;
    chooses->randomize(prng, otNumber / 8);

    std::string ch = chooses->toString();

    std::pair<bcos::bytes, bcos::bytesPointer> pairA = simplestOt0->senderGeneratePointA();
    std::pair<bcos::bytesPointer, std::vector<bcos::bytes>> parsBs =
        simplestOt1->receiverGeneratePointsB(chooses, pairA.second);
    std::vector<bcos::bytes> receiverKeys =
        simplestOt0->finishReceiver(pairA.second, parsBs.second);
    std::vector<std::array<bcos::bytes, 2>> senderKeys =
        simplestOt1->finishSender(pairA.first, pairA.second, parsBs.first);

    for (uint32_t i = 0; i < otNumber; ++i)
    {
        BOOST_CHECK(receiverKeys[i] == senderKeys[i][chooses->get(i)]);
    }
}

BOOST_AUTO_TEST_CASE(testRandomot)
{
    auto eccFactory = std::make_shared<EccCryptoFactoryImpl>();
    auto hashFactory = std::make_shared<HashFactoryImpl>();

    // test sm
    auto sm3Hash = hashFactory->createHashImpl((int8_t)ppc::protocol::HashImplName::SM3);
    auto sm2Ecc0 = eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::SM2, sm3Hash);
    auto sm2Ecc1 = eccFactory->createEccCrypto((int8_t)ppc::protocol::ECCCurve::SM2, sm3Hash);
    runRandomOT(sm2Ecc0, sm2Ecc1, sm3Hash, sm3Hash);

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
            runRandomOT(eccSet0[i], eccSet1[i], hashImpl, hashImpl);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
