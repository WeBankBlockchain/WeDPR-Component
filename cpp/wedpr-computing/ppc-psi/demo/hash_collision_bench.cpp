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
 * @file hash_collision_bench.cpp
 * @desc: bench for hash collision test
 * @author: yujiechen
 * @date 2022-12-30
 */
#include "ppc-crypto-core/src/hash/BitMixMurmurHash.h"
#include "ppc-crypto-core/src/hash/Sha256Hash.h"
#include "ppc-crypto/src/ecc/OpenSSLEccCrypto.h"
#include "ppc-crypto/src/oprf/RA2018Oprf.h"
#include "ppc-io/src/FileLineReader.h"


using namespace ppc::crypto;
using namespace ppc::io;
using namespace bcos;

void Usage(std::string const& _appName)
{
    std::cout << _appName << "[file1] [file2] [hashBits]" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        Usage(argv[0]);
        return -1;
    }
    auto file1 = argv[1];
    auto file2 = argv[2];
    int hashBits = 32;
    if (argc == 4)
    {
        hashBits = atoi(argv[3]);
    }
    if (hashBits != 32 && hashBits != 64)
    {
        std::cout << "Invalid hashBits, must be 32 or 64" << std::endl;
        return -1;
    }
    std::cout << "hash_collision_bench, file1: " << file1 << std::endl;
    std::cout << "hash_collision_bench, file2: " << file2 << std::endl;
    auto lineReader1 = std::make_shared<FileLineReader>(file1);
    auto lineReader2 = std::make_shared<FileLineReader>(file2);

    // load all data
    auto dataBatch1 = lineReader1->next(-1, DataSchema::Bytes);
    auto dataBatch2 = lineReader2->next(-1, DataSchema::Bytes);
    std::cout << "* file1 element size:" << dataBatch1->size() << std::endl;
    std::cout << "* file2 element size:" << dataBatch2->size() << std::endl;

    auto hashImpl = std::make_shared<ppc::crypto::Sha256Hash>();
    auto eccCrypto =
        std::make_shared<ppc::crypto::OpenSSLEccCrypto>(hashImpl, ppc::protocol::ECCCurve::P256);
    auto privateKey = eccCrypto->generateRandomScalar();
    auto oprf = std::make_shared<RA2018Oprf>(privateKey, eccCrypto, hashImpl);
    // full-evaluate
    std::vector<bcos::bytes> result1;
    oprf->fullEvaluate(dataBatch1, result1);
    std::vector<bcos::bytes> result2;
    oprf->fullEvaluate(dataBatch2, result2);

    // calculate the hash
    std::set<uint64_t> hashData;
    auto hasher = std::make_shared<BitMixMurmurHash>();
    for (uint64_t i = 0; i < result1.size(); i++)
    {
        hashData.insert(hasher->hash(result1.at(i), hashBits));
    }
    uint64_t collisionCount = 0;
    // check the collision
    for (uint64_t i = 0; i < result2.size(); i++)
    {
        auto hashResult = hasher->hash(result2.at(i), hashBits);
        if (hashData.count(hashResult))
        {
            collisionCount++;
            auto collisionElement = dataBatch2->get<bcos::bytes>(i);
            std::cout << "* collision: "
                      << std::string(collisionElement.begin(), collisionElement.end()) << std::endl;
        }
    }
    std::cout << "* hash_collision_bench test finished!" << std::endl;
    std::cout << "* collisionCount: " << collisionCount << std::endl;
    std::cout << "* file1 element size: " << dataBatch1->size() << std::endl;
    std::cout << "* file2 element size: " << dataBatch2->size() << std::endl;
    return 0;
}