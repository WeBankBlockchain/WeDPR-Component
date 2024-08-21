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
 * @file RedisStorage.cpp
 * @author: yujiechen
 * @date 2022-10-24
 */
#include "RedisStorage.h"
#include <bcos-utilities/BoostLog.h>

using namespace ppc::storage;
using namespace sw::redis;

bool RedisStorage::exists(const std::string& _key)
{
    // return 1 if key exists; return 0 if key doesn't exist
    return (m_redis->exists(_key) == 0 ? false : true);
}

void RedisStorage::setValue(
    const std::string& _key, const std::string& _value, int32_t _expirationSeconds)
{
    m_redis->set(_key, _value);
    // Note: when _expirationSeconds is set to -1, never trigger expiration
    if (_expirationSeconds)
    {
        expireKey(_key, _expirationSeconds);
    }
}

// std::nullopt if the key doesn't exist
std::optional<std::string> RedisStorage::getValue(const std::string& _key)
{
    OptionalString result = m_redis->get(_key);
    if (!result)
    {
        return std::nullopt;
    }
    return std::string(result.value());
}

bool RedisStorage::expireKey(const std::string& _key, uint32_t _expirationSeconds)
{
    return m_redis->expire(_key, _expirationSeconds);
}

uint64_t RedisStorage::deleteKey(const std::string& _key)
{
    return m_redis->del(_key);
}
