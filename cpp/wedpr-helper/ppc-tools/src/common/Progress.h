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
 * @file Progress.h
 * @author: shawnhe
 * @date 2023-03-10
 */

#pragma once

#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>
#include <gperftools/malloc_extension.h>
#include <unordered_set>

#define TOOLS_PROGRESS_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PPCTools")

namespace ppc::tools
{
using FlagType = std::variant<std::string, uint64_t, int64_t>;


class Progress
{
public:
    using Ptr = std::shared_ptr<Progress>;
    using ConstPtr = std::shared_ptr<const Progress>;
    Progress(bcos::ThreadPool::Ptr _threadPool) : m_threadPool(std::move(_threadPool)){};
    ~Progress()
    {
        std::unordered_set<FlagType>().swap(m_flags);
        MallocExtension::instance()->ReleaseFreeMemory();
    }

    void reset(int64_t _target, std::function<void()>&& _finalizeHandler)
    {
        TOOLS_PROGRESS_LOG(INFO) << LOG_BADGE("reset") << LOG_KV("target", _target);
        bcos::WriteGuard l(x_flags);
        m_flags.clear();
        m_target = _target;
        m_finalizeHandler = std::move(_finalizeHandler);
    }

    template <typename T>
    int64_t mark(T&& _flag)
    {
        TOOLS_PROGRESS_LOG(TRACE) << LOG_BADGE("mark") << LOG_KV("flag", _flag);
        int64_t fSize;
        std::function<void()> callback = nullptr;
        {
            bcos::WriteGuard l(x_flags);
            m_flags.insert(std::forward<T>(_flag));
            fSize = (int64_t)m_flags.size();
            if (fSize == m_target)
            {
                callback = std::move(m_finalizeHandler);
            }
        }

        if (callback)
        {
            TOOLS_PROGRESS_LOG(INFO)
                << LOG_BADGE("handle finalizeHandler") << LOG_KV("target", m_target);

            if (m_threadPool)
            {
                m_threadPool->enqueue([callback] { callback(); });
            }
            else
            {
                callback();
            }
        }

        return fSize;
    }

    int64_t current()
    {
        bcos::ReadGuard l(x_flags);
        return (int64_t)m_flags.size();
    }


private:
    bcos::ThreadPool::Ptr m_threadPool;
    mutable bcos::SharedMutex x_flags;
    std::unordered_set<FlagType> m_flags;
    int64_t m_target{-1};
    std::function<void()> m_finalizeHandler;
};

}  // namespace ppc::tools
