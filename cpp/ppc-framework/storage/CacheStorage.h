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
 * @file StorageCache.h
 * @author: shawnhe
 * @date 2022-10-21
 */

#pragma once
#include "ppc-framework/protocol/Protocol.h"
#include <bcos-utilities/Log.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace ppc::storage
{
struct CacheStorageConfig
{
    ppc::protocol::CacheType type;
    std::string proxy;
    std::string obServer;
    std::string cluster;
    std::string user;
    std::string host;
    uint16_t port;
    std::string password;
    uint16_t database;
    uint16_t pool;
    // default connection-timeout is 500ms
    uint16_t connectionTimeout = 500;
    // default socket-timeout is 500ms
    uint16_t socketTimeout = 500;

    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("type", (int)type) << LOG_KV("proxy", proxy) << LOG_KV("obServer", obServer)
            << LOG_KV("cluster", cluster) << LOG_KV("host", host) << LOG_KV("port", port)
            << LOG_KV("database", database) << LOG_KV("poolSize", pool)
            << LOG_KV("connectionTimeout", connectionTimeout)
            << LOG_KV("socketTimeout", socketTimeout);
        return oss.str();
    }
};

class CacheStorage
{
public:
    using Ptr = std::shared_ptr<CacheStorage>;
    CacheStorage() = default;
    virtual ~CacheStorage() {}

    /**
     * @brief: check whether the key exists
     * @param _key: key
     * @return whether the key exists
     */
    virtual bool exists(const std::string& _key) = 0;

    /**
     * @brief: set key value
     * @param _expirationTime: timeout of key, in seconds, when set to -1, never expire
     */
    virtual void setValue(
        const std::string& _key, const std::string& _value, int32_t _expirationSeconds = -1) = 0;

    /**
     * @brief: get value by key
     * @param _key: key
     * @return std::nullopt if the key doesn't exist;
     */
    virtual std::optional<std::string> getValue(const std::string& _key) = 0;

    /**
     * @brief: set a timeout on key
     * @param _expirationTime: timeout of key, seconds
     * @return whether setting is successful
     */
    virtual bool expireKey(const std::string& _key, uint32_t _expirationSeconds) = 0;

    /**
     * @brief: delete key
     * @param _key: key
     * @return the number of key deleted
     */
    virtual uint64_t deleteKey(const std::string& _key) = 0;
};


class CacheStorageFactory
{
public:
    using Ptr = std::shared_ptr<CacheStorageFactory>;
    CacheStorageFactory() = default;
    virtual ~CacheStorageFactory() = default;

    virtual CacheStorage::Ptr createCacheStorage(CacheStorageConfig const& _config) = 0;
};

}  // namespace ppc::storage
