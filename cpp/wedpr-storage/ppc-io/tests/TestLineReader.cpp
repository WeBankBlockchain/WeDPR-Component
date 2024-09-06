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
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/test/unit_test.hpp>

using namespace ppc::io;
using namespace bcos;
using namespace bcos::test;

namespace ppc::test
{
BOOST_FIXTURE_TEST_SUITE(lineReaderTest, TestPromptFixture)

void testFileLineReaderFunc(FileLineReaderFactory::Ptr _factory, std::string const& _filePath,
    char _lineSpliter, uint64_t _mmapGranularity, uint64_t _fileTotalLine,
    std::vector<std::string> const& _expectedData, uint64_t _batchSize)
{
    if (_mmapGranularity % getpagesize() != 0)
    {
        BOOST_CHECK_THROW(_factory->createLineReader(_filePath, _mmapGranularity, _lineSpliter),
            InvalidMmapGranularity);
        return;
    }
    auto lineReader = _factory->createLineReader(_filePath, _mmapGranularity, _lineSpliter);
    // case1: read all data from the file
    auto dataBatch = lineReader->next(-1);
    BOOST_CHECK(dataBatch->size() == _fileTotalLine);
    std::cout << "#### dataBatch size: " << dataBatch->size() << std::endl;

    // check the data
    uint64_t offset = 0;
    for (auto const& item : _expectedData)
    {
        BOOST_CHECK(item == dataBatch->get<std::string>(offset));
        offset++;
    }
    // try to read again while the file has been read-finished
    BOOST_CHECK(nullptr == lineReader->next(1));

    // case2: read _batchSize everyTime
    lineReader = _factory->createLineReader(_filePath, _mmapGranularity, _lineSpliter);
    uint64_t readedLine = 0;
    std::vector<DataBatch::Ptr> dataBatches;
    while (true)
    {
        dataBatch = lineReader->next(_batchSize);
        if (!dataBatch)
        {
            break;
        }
        readedLine += dataBatch->size();
        dataBatches.emplace_back(dataBatch);
    }
    BOOST_CHECK(readedLine == _expectedData.size());
    // check the dataBatch
    offset = 0;
    for (auto const& batch : dataBatches)
    {
        for (uint64_t i = 0; i < batch->size(); i++)
        {
            BOOST_CHECK(batch->get<std::string>(i) == _expectedData[offset]);
            offset++;
        }
    }
}
BOOST_AUTO_TEST_CASE(testDataBatch)
{
    auto dataBatch = std::make_shared<DataBatch>();
    uint64_t dataSize = 1002;
    std::vector<std::string> stringData;
    std::vector<uint64_t> intData;
    uint64_t expectedStringCapacity = 0;
    for (uint64_t i = 0; i < dataSize; i++)
    {
        auto str = std::to_string(i);
        expectedStringCapacity += str.size();
        stringData.emplace_back(str);
        intData.emplace_back(i);
    }
    // setData and check capacity
    auto tmpData = stringData;
    dataBatch->setData(std::move(tmpData));
    std::cout << "#### capacityBytes" << dataBatch->capacityBytes() << std::endl;
    std::cout << "### expectedStringCapacity: " << expectedStringCapacity << std::endl;
    BOOST_CHECK(dataBatch->capacityBytes() == expectedStringCapacity);
    // append
    for (uint64_t i = 0; i < stringData.size(); i++)
    {
        BOOST_CHECK(dataBatch->get<std::string>(i) == stringData.at(i));
        BOOST_CHECK(dataBatch->elementSize<std::string>(i) == stringData.at(i).size());
        auto tmp = stringData.at(i);
        dataBatch->append<std::string>(std::move(tmp));
    }
    BOOST_CHECK(dataBatch->capacityBytes() == 2 * expectedStringCapacity);

    // appendToLine
    auto orgSize = dataBatch->elementSize<std::string>(dataBatch->size() - 1);
    auto data = stringData.at(0);
    dataBatch->appendToLine<std::string>(std::move(data));
    BOOST_CHECK(dataBatch->elementSize<std::string>(dataBatch->size() - 1) ==
                (stringData.at(0).size() + orgSize));
    // test set
    std::string resetData = "reset";
    auto tmp = resetData;
    dataBatch->set<std::string>(0, std::move(tmp));
    BOOST_CHECK(resetData == dataBatch->get<std::string>(0));

    auto intDataBatch = std::make_shared<DataBatch>();
    auto tmpIntData = intData;
    intDataBatch->setData(std::move(tmpIntData));
    BOOST_CHECK(intDataBatch->capacityBytes() == dataSize * sizeof(uint64_t));
    for (uint64_t i = 0; i < intData.size(); i++)
    {
        BOOST_CHECK(intDataBatch->get<uint64_t>(i) == intData.at(i));
        BOOST_CHECK(intDataBatch->elementSize<uint64_t>(i) == sizeof(intData.at(i)));
        auto tmp = intData.at(i);
        intDataBatch->append<uint64_t>(std::move(tmp));
    }
    BOOST_CHECK(intDataBatch->capacityBytes() == 2 * dataSize * sizeof(uint64_t));

    // the data-schema not set
    BOOST_CHECK_THROW(intDataBatch->getBytes(0), DataSchemaNotSetted);
    intDataBatch->setDataSchema(DataSchema::Uint);

    auto expectedData = boost::lexical_cast<std::string>(intData.at(0));
    BOOST_CHECK(intDataBatch->getBytes(0) == bcos::bytes(expectedData.begin(), expectedData.end()));
}
BOOST_AUTO_TEST_CASE(testFileLineReader)
{
    std::string filePath = "../../../../wedpr-storage/ppc-io/tests/data/testData";
    uint64_t fileTotalLine = 10000;
    std::vector<std::string> fileData;
    for (uint64_t i = 1; i <= fileTotalLine; i++)
    {
        fileData.emplace_back(std::to_string(i));
    }
    auto factory = std::make_shared<FileLineReaderFactory>();
    // case1: invalid mmap-granularity
    testFileLineReaderFunc(factory, filePath, '\n', 1024, fileTotalLine, fileData, 100);

    // case2: valid mmap-granularity
    auto pageSize = getpagesize();
    auto startT = utcSteadyTime();
    testFileLineReaderFunc(factory, filePath, '\n', pageSize, fileTotalLine, fileData, 100);
    std::cout << "### timecost with 1-page mmap-granularity:" << (utcSteadyTime() - startT) << " ms"
              << ", pageSize:" << pageSize << std::endl;

    // case3: large mmap-granularity
    startT = bcos::utcSteadyTime();
    testFileLineReaderFunc(
        factory, filePath, '\n', 500 * 1024 * pageSize, fileTotalLine, fileData, 100);
    std::cout << "### timecost with 500K-page mmap-granularity:" << (utcSteadyTime() - startT)
              << " ms"
              << ", pageSize:" << pageSize << std::endl;

    // case4: the windows file
    auto windowsFilePath = "../../../../wedpr-storage/ppc-io/tests/data/windows_file.txt";
    auto lineReader = factory->createLineReader(windowsFilePath, pageSize, '\n');
    auto windowsDataBatch = lineReader->next(-1, DataSchema::Bytes);

    auto linuxFilePath = "../../../../wedpr-storage/ppc-io/tests/data/linux_file.txt";
    lineReader = factory->createLineReader(linuxFilePath, pageSize, '\n');
    auto linuxDataBatch = lineReader->next(-1, DataSchema::Bytes);
    BOOST_CHECK(windowsDataBatch->size() == linuxDataBatch->size());
    for (uint64_t i = 0; i < windowsDataBatch->size(); i++)
    {
        BOOST_CHECK(windowsDataBatch->get<bcos::bytes>(i) == linuxDataBatch->get<bcos::bytes>(i));
    }
}
BOOST_AUTO_TEST_SUITE_END()
}  // namespace ppc::test
