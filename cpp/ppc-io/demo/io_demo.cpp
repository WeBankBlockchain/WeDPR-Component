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
 * @file io_demo.cpp
 * @desc: demo for io
 * @author: yujiechen
 * @date 2022-10-19
 */
#include "ppc-io/src/FileLineReader.h"
#include "ppc-io/src/FileLineWriter.h"
#include <bcos-utilities/Common.h>

using namespace bcos;
using namespace ppc::io;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0]
                  << ": \n - [fileToRead(required)]\n - mmapGranularity: default is 500MB \n - "
                     "readPerBatch(default read all)\n - writeLines: default is 1000w";
        return -1;
    }
    std::string fileToRead = argv[1];
    uint64_t mmapGranularity = 500 * 1024 * 1024;
    if (argc == 3)
    {
        mmapGranularity = atol(argv[2]);
    }
    int16_t readPerBatch = -1;
    if (argc == 4)
    {
        readPerBatch = atoi(argv[3]);
    }
    uint64_t dataToWrite = 10000000;
    if (argc == 5)
    {
        dataToWrite = atoi(argv[4]);
    }
    std::cout << "fileToRead:\t" << fileToRead << std::endl;
    std::cout << "mmapGranularity:\t" << mmapGranularity << std::endl;
    if (readPerBatch == -1)
    {
        std::cout << "readPerBatch:\t readAll" << std::endl;
    }
    else
    {
        std::cout << "readPerBatch:\t" << readPerBatch << std::endl;
    }

    std::cout << "----- parseFileBegin----------";
    auto factory = std::make_shared<FileLineReaderFactory>();
    auto lineReader = factory->createLineReader(fileToRead, mmapGranularity);
    auto startT = utcSteadyTime();
    uint64_t lineSize = 0;
    while (true)
    {
        auto dataBatch = lineReader->next(readPerBatch);
        if (!dataBatch)
        {
            break;
        }
        lineSize += dataBatch->size();
    }
    std::cout << "timecost:\t" << (utcSteadyTime() - startT) << "ms " << std::endl;
    std::cout << "fileSize:\t" << lineReader->capacity() << std::endl;
    std::cout << "lines:\t\t" << lineSize << std::endl;
    std::cout << "----- parseFileFinished----------";
    std::cout << "------ test writer --------------)";
    uint64_t batchSize = 10;
    uint64_t dataGranularity = dataToWrite / batchSize;
    std::vector<bytes> dataVec;
    for (uint64_t j = 0; j < dataGranularity; j++)
    {
        std::string data = "234k9lwekrwejrlewkrjwe";
        dataVec.emplace_back(bcos::bytes(data.begin(), data.end()));
    }
    for (uint64_t i = 0; i < 10; i++)
    {
        auto writer = std::make_shared<FileLineWriter>("output.txt", false);
        auto dataBatch = std::make_shared<ppc::io::DataBatch>();
        auto tmpData = dataVec;
        dataBatch->setData(std::move(tmpData));
        writer->writeLine(dataBatch, DataSchema::Bytes, "\n");
        writer->flush();
        writer->close();
    }
}
