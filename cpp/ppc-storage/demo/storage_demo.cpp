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
 * @file storage_demo.cpp
 * @desc: demo for storage
 * @author: yujiechen
 * @date 2022-10-24
 */
#include "ppc-storage/src/CacheStorageFactoryImpl.h"
#include <bcos-utilities/Common.h>
#include <boost/chrono/include.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>

using namespace ppc::storage;
using namespace ppc::protocol;
using namespace bcos;
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0]
                  << "\nredis: 0 ip port 123456\n proxy obServer cluster user password";
        return -1;
    }
    try
    {
        CacheStorageConfig cacheStorageConfig;

        if (atoi(argv[1]) == 0)
        {
            // redis
            cacheStorageConfig.type = CacheType::Redis;
            cacheStorageConfig.host = argv[2];
            cacheStorageConfig.port = atoi(argv[3]);
            cacheStorageConfig.password = argv[4];
            cacheStorageConfig.database = 1;
        }
        else
        {
            std::cout << "Usage: " << argv[0]
                      << "\nredis: 0 ip port 123456\n proxy obServer cluster user password";
            return -1;
        }

        CacheStorageFactoryImpl::Ptr factory = std::make_shared<CacheStorageFactoryImpl>();
        auto cacheClient = factory->createCacheStorage(cacheStorageConfig);

        // set and get data
        std::cout << "#### start cache test ####" << std::endl;
        auto startT = utcSteadyTime();
        std::cout << "check setValue/getValue/exists..." << std::endl;
        for (int i = 0; i < 1000; i++)
        {
            auto key = std::to_string(i);
            auto value = "value" + key;
            cacheClient->setValue(key, value);
            auto result = cacheClient->getValue(key);
            BOOST_CHECK(result.has_value() && result == value);
            // check exists
            BOOST_CHECK(cacheClient->exists(key));
        }
        std::cout << "setValue/getValue/exists success, time: " << (utcSteadyTime() - startT)
                  << "ms" << std::endl;
        std::cout << "check non-exists key..." << std::endl;
        // check non-exists data
        startT = utcSteadyTime();
        for (int i = 10000; i < 500; i++)
        {
            auto key = std::to_string(i);
            auto result = cacheClient->getValue(key);
            BOOST_CHECK(!result.has_value());
            BOOST_CHECK(!cacheClient->exists(key));
        }
        std::cout << "check non-exists key success, time: " << (utcSteadyTime() - startT) << "ms"
                  << std::endl;
        // expire all the keys with 1s expiration-time
        startT = utcSteadyTime();
        std::cout << "check expireKey key..." << std::endl;
        for (int i = 0; i < 1000; i++)
        {
            auto key = std::to_string(i);
            cacheClient->expireKey(key, 1);
        }
        std::cout << "check expireKey key success, time: " << (utcSteadyTime() - startT) << "ms"
                  << std::endl;
        // sleep 2s
        std::cout << "sleep 2 seconds to wait for key-expiration" << std::endl;
        boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));
        std::cout << "wait-finished" << std::endl;
        // check the key
        startT = utcSteadyTime();
        std::cout << "check the key-expiration..." << std::endl;
        for (int i = 0; i < 1000; i++)
        {
            auto key = std::to_string(i);
            auto result = cacheClient->getValue(key);
            BOOST_CHECK(!result.has_value());
            BOOST_CHECK(!cacheClient->exists(key));
        }
        std::cout << "check the key-expiration success, time:" << (utcSteadyTime() - startT)
                  << std::endl;
        std::cout << "#### end cache test ####" << std::endl;
    }
    catch (std::exception const& e)
    {
        std::cout << "access cache error: \n" << boost::diagnostic_information(e) << std::endl;
    }
}