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
 * @file TestCEMService.cpp
 * @author: caryliao
 * @date 2022-11-19
 */
#include "ppc-cem/src/CEMService.h"
#include "ppc-cem/src/Common.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::cem;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(cMServiceTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(testCEMService)
{
    auto cemService = std::make_shared<CEMService>();
    std::string configPath{"../../../../wedpr-computing/ppc-cem/tests/data/config.ini"};
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(configPath, pt);
    // load the cm config
    auto ppcConfig = std::make_shared<PPCConfig>();
    ppcConfig->loadCEMConfig(pt);
    auto cemConfig = ppcConfig->cemConfig();
    auto storageConfig = ppcConfig->storageConfig();
    cemService->setCEMConfig(cemConfig);
    cemService->setStorageConfig(storageConfig);

    // d1 encrypted
    std::string all_datasets[3] = {"d1", "d2", "d3"};
    for (int i = 0; i < 3; i++)
    {
        std::string datasetId = all_datasets[i];
        std::string encrypted_file =
            cemConfig.datasetFilePath + "/" + datasetId + cemConfig.ciphertextSuffix;
        if (boost::filesystem::exists(encrypted_file))
        {
            boost::filesystem::remove(encrypted_file);
        }
        auto linereader =
            cemService->initialize_lineReader(datasetId, ppc::protocol::DataResourceType::FILE);
        auto linewriter =
            cemService->initialize_lineWriter(datasetId, ppc::protocol::DataResourceType::FILE);
        cemService->doEncryptDataset(linereader, linewriter);
    }

    // d1 matched
    Json::Value request2(Json::arrayValue);

    Json::Value param1;
    param1["dataset_id"] = "d1-encrypted";
    Json::Value matchField1;
    matchField1["x1"] =
        "b53d3973005bd84da8367d4d247946b00ed269454d8799608631262862b3ee2a912fa42e1e71af77236ad851ea"
        "e44f0c91a674f59469a47c0d8cac7385009d22f012f44f3477ad254a91f3ef07b7a2fc2c78012b2538a9052ce0"
        "53dd884ffe2914f7027ae7d545b050426f12f8f225a058a4c609773d0ae7cc73e103954790b7d52a398a88e42e"
        "dd8a006cf1325828b9";
    matchField1["x2"] =
        "8357af78f88dfbe857bc4eb6801b83714d3f3127bdd4e68bbc17444fe8783686dd36ab44f9b80fb6c173ef0fb1"
        "8c22e093216884966cd61c3723c987d91471c76a449d18070358862ebdc50c5d326035c1c188b5642baba5f3f6"
        "37a7b5aec5fc054fe607cd0fad22e75468c47dd30a74af05f42a84ba73adf7f277fe94a8f4cd42b23563bbbe42"
        "a1e493f44a65f1c52c";
    param1["match_field"] = matchField1;

    Json::Value param2;
    param2["dataset_id"] = "d2-encrypted";
    Json::Value matchField2;
    matchField2["x1"] =
        "971a80d65f1983e14a5bd33347237adadd8e8586ac766d1e06e68bde9957032e009cab0c3501cb3d620aefab4c"
        "063fc9b43d2bedd7b0ddc9c5b24d53fa2e35cd41b689350e417df8cb24ce9e585501c04c9535bd2525d02f5397"
        "c60ed353358916b585666d0f124df64269151c1f218df4becc79b237124ddda84b64bfa0230b5b012a9a87d28c"
        "629d635b0ae84467ab";
    matchField2["x2"] =
        "a8374c5e1ca1ef7944ac904491b94d0dafc3c8bb81feaf4f7bff24d9083f8cb459a3181aa454556558ba80ffb9"
        "7d91cdaaa17e4e27a75d3835ab69963429aee031b54aa5ecdadbea45566452da568b51aed0b188d1d0ac86ed5e"
        "5f665239094e156c1eec61d1d3aa66d98eb004ef3cb870a79cb1fe9ac7c698dcf360d85bfd066b9fd1c9642dd9"
        "6960c9bd41027e368b";
    param2["match_field"] = matchField2;

    Json::Value param3;
    param3["dataset_id"] = "d3-encrypted";
    Json::Value matchField3;
    matchField3["x1"] =
        "acf497223d82cf3347d19f00ba4e5e1139c1897e05667186d9c766fa482a4aacaf66b6aecd335bc437e637f8d9"
        "b4327f8ed030aec4e047d32cde0b00421a3f31c1857dac3bdfb99700fea524fe3873bfb66809a7ea48f3df251e"
        "41dbe27685f10fd525dd18b4e3eabb5e89bba75593439cc73ff19b5afd9f45a2a89407353b0d2b5af875f270dd"
        "3d865776216ce194b2";
    matchField3["x2"] =
        "a5e604a12f2f06848b626e16cd13d8f74cf866c3963074715b8251ff77fe1c797e40c12785ac872432d814e8df"
        "7cd6ad8383c2605111479ddb47941ae27e5e3eb29d1bfd2f2f92b6a7f3c312ddefacf12fc016776d96fbd9998d"
        "99399359b11d10641c1d724af6a42659ce0f254a2258af388e8a89c7e7b420a5daa9efb304cdae0ec1c328c6f8"
        "5ff3c9fdcbbe29cce8";
    param3["match_field"] = matchField3;

    request2.append(param1);
    request2.append(param2);
    request2.append(param3);

    Json::Value response2(Json::arrayValue);
    response2.resize((Json::ArrayIndex)request2.size());
    cemService->makeCiphertextEqualityMatch(
        request2, response2, ppc::protocol::DataResourceType::FILE);
    BOOST_CHECK(response2.size() == 3);
    uint64_t matchResult = 50;
    BOOST_CHECK(response2[(Json::ArrayIndex)0]["dataset_id"].asString() == "d1-encrypted");
    BOOST_CHECK(response2[(Json::ArrayIndex)0]["match_count"]["x1"].asUInt64() == matchResult);
    BOOST_CHECK(response2[(Json::ArrayIndex)0]["match_count"]["x2"].asUInt64() == matchResult);

    BOOST_CHECK(response2[(Json::ArrayIndex)1]["dataset_id"].asString() == "d2-encrypted");
    BOOST_CHECK(response2[(Json::ArrayIndex)1]["match_count"]["x1"].asUInt64() == matchResult);
    BOOST_CHECK(response2[(Json::ArrayIndex)1]["match_count"]["x2"].asUInt64() == matchResult);

    BOOST_CHECK(response2[(Json::ArrayIndex)2]["dataset_id"].asString() == "d3-encrypted");
    BOOST_CHECK(response2[(Json::ArrayIndex)2]["match_count"]["x1"].asUInt64() == matchResult);
    BOOST_CHECK(response2[(Json::ArrayIndex)2]["match_count"]["x2"].asUInt64() == matchResult);
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
