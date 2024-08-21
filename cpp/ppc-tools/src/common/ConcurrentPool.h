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
 * @file ConcurrentPool.h
 * @author: zachma
 * @date 2023-8-30
 */

#pragma once
#include <condition_variable>
#include <map>
#include <mutex>

namespace ppc::tools
{
template <typename _K, typename _V, typename PoolKV = std::map<_K, _V>>
class ConcurrentPool
{
public:
    void insert(_K _key, _V _elem)
    {
        {
            std::lock_guard<decltype(x_mutex)> guard{x_mutex};
            m_pool.insert(std::pair<_K, _V>(_key, _elem));
        }
        m_cv.notify_one();
    }

    bool empty()
    {
        boost::unique_lock<boost::mutex> lock{x_mutex};
        return m_pool.empty();
    }

    _V pop(_K _key)
    {
        boost::unique_lock<boost::mutex> lock{x_mutex};
        m_cv.wait(lock, [this, _key] { return exist(_key); });
        auto v = m_pool[_key];
        m_pool.erase(_key);
        return v;
    }

    std::pair<bool, _V> tryPop(_K _key, int milliseconds)
    {
        boost::unique_lock<boost::mutex> lock{x_mutex};
        // in consideration that when the system time has been changed,
        // the process maybe stucked in 'wait_for'
        auto ret = m_cv.wait_for(
            lock, boost::chrono::milliseconds(milliseconds), [this, _key] { return exist(_key); });
        if (!ret)
        {
            return std::make_pair(false, _V());
        }
        auto item = m_pool[_key];
        m_pool.erase(_key);
        return std::make_pair(ret, item);
    }

    bool exist(_K _key) { return m_pool.find(_key) != m_pool.end(); }

private:
    PoolKV m_pool;
    boost::mutex x_mutex;
    boost::condition_variable m_cv;
};
}  // namespace ppc::tools