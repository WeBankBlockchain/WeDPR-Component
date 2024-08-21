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
 * @file RedisStorage.h
 * @author: yujiechen
 * @date 2022-10-24
 */
#pragma once

#include "ppc-framework/storage/CacheStorage.h"
#include <sw/redis++/redis++.h>

namespace ppc::storage
{

class RedisStorage : public CacheStorage
{
public:
    using Ptr = std::shared_ptr<RedisStorage>;
    RedisStorage(const sw::redis::ConnectionOptions& _connectionOpts,
        const sw::redis::ConnectionPoolOptions& _poolOptions)
    {
        m_redis = std::make_shared<sw::redis::Redis>(_connectionOpts, _poolOptions);
    }
    ~RedisStorage() override {}

public:
    /// Note: all these interfaces throws exception when error happened
    /**
     * @brief: check whether the key exists
     * @param _key: key
     * @return whether the key exists
     */
    bool exists(const std::string& _key) override;

    /**
     * @brief: set key value
     * @param _expirationTime: timeout of key, seconds
     */
    void setValue(const std::string& _key, const std::string& _value,
        int32_t _expirationSeconds = -1) override;

    /**
     * @brief: get value by key
     * @param _key: key
     * @return value
     */
    std::optional<std::string> getValue(const std::string& _key) override;

    /**
     * @brief: set a timeout on key
     * @param _expirationTime: timeout of key, ms
     * @return whether setting is successful
     */
    bool expireKey(const std::string& _key, uint32_t _expirationTime) override;

    /**
     * @brief: delete key
     * @param _key: key
     * @return the number of key deleted
     */
    uint64_t deleteKey(const std::string& _key) override;

private:
    std::shared_ptr<sw::redis::Redis> m_redis;
};

}  // namespace ppc::storage