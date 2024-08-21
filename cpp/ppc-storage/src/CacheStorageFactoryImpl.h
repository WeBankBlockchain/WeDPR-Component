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
 * @file CacheStorageFactoryImpl.h
 * @author: shawnhe
 * @date 2023-03-13
 */
#pragma once

#include "Common.h"
#include "ppc-framework/storage/CacheStorage.h"
#include "redis/RedisStorage.h"

namespace ppc::storage
{
class CacheStorageFactoryImpl : public ppc::storage::CacheStorageFactory
{
public:
    using Ptr = std::shared_ptr<CacheStorageFactoryImpl>;

public:
    CacheStorageFactoryImpl() = default;
    ~CacheStorageFactoryImpl() = default;

public:
    CacheStorage::Ptr createCacheStorage(CacheStorageConfig const& _config)
    {
        switch (_config.type)
        {
        case ppc::protocol::CacheType::Redis:
        {
            STORAGE_LOG(INFO) << LOG_DESC("buildRedisCache");
            sw::redis::ConnectionOptions connection_options;
            connection_options.host = _config.host;
            connection_options.port = _config.port;
            connection_options.password = _config.password;
            connection_options.db = _config.database;

            // *0ms* by default.
            connection_options.connect_timeout =
                std::chrono::milliseconds(_config.connectionTimeout);
            // Optional. Timeout before we successfully send request to or receive response from
            // redis. By default, the timeout is 0ms, i.e. never timeout and block until we send or
            // receive successfuly. NOTE: if any command is timed out, we throw a TimeoutError
            // exception. 0ms by default
            connection_options.socket_timeout = std::chrono::milliseconds(_config.socketTimeout);
            connection_options.keep_alive = true;

            sw::redis::ConnectionPoolOptions pool_options;
            pool_options.size = _config.pool;

            return std::make_shared<RedisStorage>(connection_options, pool_options);
        }
        default:
        {
            BOOST_THROW_EXCEPTION(
                UnsupportedCacheStorage() << bcos::errinfo_comment(
                    "unsupported cache type: " + std::to_string((int)_config.type)));
        }
        }
    }
};
}  // namespace ppc::storage