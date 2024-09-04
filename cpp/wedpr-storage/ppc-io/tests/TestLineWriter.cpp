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
 * @file TestLineReader.cpp
 * @author: yujiechen
 * @date 2022-10-21
 */

#include "ppc-io/src/Common.h"
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::io;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(lineWriterTest, TestPromptFixture)
BOOST_AUTO_TEST_CASE(testFileLineWriter)
{
    auto dataBatch = std::make_shared<DataBatch>();
    for (uint64_t i = 0; i < 10000; i++)
    {
        dataBatch->append(std::to_string(i));
    }
    std::string fileName = "testWriter";
    auto writer = std::make_shared<FileLineWriter>(fileName, true);
    writer->writeLine(dataBatch, DataSchema::String, "\n");
    writer->close();
    // check the content use FileLineReader
    auto reader = std::make_shared<FileLineReader>(fileName);
    auto readedData = reader->next(-1);
    BOOST_CHECK(readedData->size() == dataBatch->size());
    for (uint64_t i = 0; i < readedData->size(); i++)
    {
        BOOST_CHECK(readedData->get<std::string>(i) == dataBatch->get<std::string>(i));
    }
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
