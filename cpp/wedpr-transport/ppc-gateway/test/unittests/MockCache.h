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
 * @file MockCache.h
 * @author: shawnhe
 * @date 2022-10-31
 */
#pragma once
#include "ppc-framework/storage/CacheStorage.h"
#include <unordered_map>

namespace ppc::mock
{
class MockCache : public storage::CacheStorage
{
public:
    using Ptr = std::shared_ptr<MockCache>;
    MockCache() = default;
    ~MockCache() override {}

    /// Note: all these interfaces throws exception when error happened
    /**
     * @brief: check whether the key exists
     * @param _key: key
     * @return whether the key exists
     */
    bool exists(const std::string& _key) override { return m_kv.find(_key) != m_kv.end(); }

    /**
     * @brief: set key value
     * @param _expirationTime: timeout of key, seconds
     */
    void setValue(const std::string& _key, const std::string& _value,
        int32_t _expirationSeconds = -1) override
    {
        m_kv.emplace(_key, _value);
    }

    /**
     * @brief: get value by key
     * @param _key: key
     * @return value
     */
    std::optional<std::string> getValue(const std::string& _key) override
    {
        auto it = m_kv.find(_key);
        if (it == m_kv.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    /**
     * @brief: set a timeout on key
     * @param _expirationTime: timeout of key, ms
     * @return whether setting is successful
     */
    bool expireKey(const std::string& _key, uint32_t _expirationTime) override { return true; }

    /**
     * @brief: delete key
     * @param _key: key
     * @return the number of key deleted
     */
    uint64_t deleteKey(const std::string& _key) override { return m_kv.erase(_key); }

private:
    std::unordered_map<std::string, std::optional<std::string>> m_kv;
};

}  // namespace ppc::mock