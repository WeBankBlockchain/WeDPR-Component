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
 * @file TestSenderDB.cpp
 * @author: shawnhe
 * @date 2022-11-25
 */

#include "DataTools.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto/src/ecc/Ed25519EccCrypto.h"
#include "ppc-crypto/src/oprf/EcdhOprf.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-psi/src/labeled-psi/core/LabeledPSIParams.h"
#include "ppc-psi/src/labeled-psi/core/SenderDB.h"
#include "protocol/src/JsonTaskImpl.h"
#include "test-utils/TaskMock.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>
using namespace ppc::psi;
using namespace bcos;
using namespace bcos::test;
using namespace ppc::crypto;
using namespace ppc::protocol;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(SenderDBTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(TestPsiParams)
{
    auto params1 = getPsiParams(1024);
    BOOST_CHECK(params1.table_params().max_items_per_bin == 42);
    BOOST_CHECK(params1.item_params().felts_per_item == 6);
    BOOST_CHECK(params1.seal_params().poly_modulus_degree() == 2048);
    BOOST_CHECK(params1.query_params().ps_low_degree == 0);
    auto params2 = getPsiParams(360000);
    BOOST_CHECK(params2.table_params().max_items_per_bin == 228);
    BOOST_CHECK(params2.item_params().felts_per_item == 6);
    BOOST_CHECK(params2.seal_params().poly_modulus_degree() == 8192);
    BOOST_CHECK(params2.query_params().ps_low_degree == 0);
    auto params3 = getPsiParams(2000000);
    BOOST_CHECK(params3.table_params().max_items_per_bin == 782);
    BOOST_CHECK(params3.item_params().felts_per_item == 4);
    BOOST_CHECK(params3.seal_params().poly_modulus_degree() == 8192);
    BOOST_CHECK(params3.query_params().ps_low_degree == 26);
    auto params4 = getPsiParams(20000000);
    BOOST_CHECK(params4.table_params().max_items_per_bin == 8100);
    BOOST_CHECK(params4.item_params().felts_per_item == 4);
    BOOST_CHECK(params4.seal_params().poly_modulus_degree() == 8192);
    BOOST_CHECK(params4.query_params().ps_low_degree == 310);
}

BOOST_AUTO_TEST_CASE(TestSenderDB)
{
    uint32_t count = (1 << 10);
    auto items = std::make_shared<io::DataBatch>();
    items->setDataSchema(ppc::io::DataSchema::Bytes);
    auto labels = std::make_shared<io::DataBatch>();
    labels->setDataSchema(ppc::io::DataSchema::Bytes);
    genItemsLabels(items, labels, count);
    BOOST_CHECK(items->size() == count);

    auto hashImpl = std::make_shared<crypto::Sha256Hash>();
    auto eccImpl = std::make_shared<crypto::Ed25519EccCrypto>();
    auto oprfServer = std::make_shared<crypto::EcdhOprfServer>(256, hashImpl, eccImpl);
    auto senderDB = std::make_shared<SenderDB>(getPsiParams(count), oprfServer, 16, 16, false);

    senderDB->setData(items, labels);
    BOOST_CHECK(senderDB->getItemCount() == count);
}

BOOST_AUTO_TEST_CASE(TestSenderCache)
{
    uint32_t count = (1 << 15);
    auto items = std::make_shared<io::DataBatch>();
    items->setDataSchema(ppc::io::DataSchema::Bytes);
    auto labels = std::make_shared<io::DataBatch>();
    labels->setDataSchema(ppc::io::DataSchema::Bytes);
    genItemsLabels(items, labels, count);
    BOOST_CHECK(items->size() == count);

    auto hashImpl = std::make_shared<crypto::Sha256Hash>();
    auto eccImpl = std::make_shared<crypto::Ed25519EccCrypto>();
    auto oprfServer = std::make_shared<crypto::EcdhOprfServer>(256, hashImpl, eccImpl);
    auto senderDB = std::make_shared<SenderDB>(getPsiParams(count), oprfServer, 16, 16, false);

    senderDB->setData(items, labels);

    bcos::bytes out;
    senderDB->saveToBytes(out);
    auto newSenderDB = SenderDB::loadFromBytes(oprfServer, out);

    BOOST_CHECK(newSenderDB->getItemCount() == count);
    BOOST_CHECK(senderDB->getParams().to_string() == newSenderDB->getParams().to_string());
    BOOST_CHECK(senderDB->isCompressed() == newSenderDB->isCompressed());
    BOOST_CHECK(senderDB->getItemCount() == newSenderDB->getItemCount());
    BOOST_CHECK(senderDB->getHashedItems().size() == newSenderDB->getHashedItems().size());
    BOOST_CHECK(senderDB->getBinBundleCount() == newSenderDB->getBinBundleCount());
}


BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test