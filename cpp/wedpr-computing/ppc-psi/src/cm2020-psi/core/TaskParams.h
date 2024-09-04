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
 * @file TaskParams.h
 * @author: shawnhe
 * @date 2022-12-9
 */

#pragma once
#include "../Common.h"
#include <bcos-utilities/Error.h>
#include <json/json.h>

namespace ppc::psi
{
class TaskParams
{
public:
    using Ptr = std::shared_ptr<TaskParams>;
    TaskParams(std::string_view _param)
    {
        if (_param.empty())
        {
            return;
        }

        Json::Reader reader;
        Json::Value result;
        if (!reader.parse(_param.begin(), _param.end(), result))
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR(
                (int)CM2020PSIRetCode::INVALID_TASK_PARAM, "invalid task param: invalid json"));
        }
        if (!result.isArray())
        {
            BOOST_THROW_EXCEPTION(BCOS_ERROR((int)CM2020PSIRetCode::INVALID_TASK_PARAM,
                "invalid task param:: the param must be array"));
        }
        if (!result.empty())
        {
            int enableHighPerformance = result[0].asInt();
            m_enableHighPerformance = enableHighPerformance > 0;
            m_bucketNumber = HIGH_PERFORMANCE_BUCKET_NUMBER;
        }
        if (result.size() > 1)
        {
            uint16_t bucketNumber = result[1].asInt();
            if (bucketNumber >= HIGH_PERFORMANCE_BUCKET_NUMBER && bucketNumber <= MAX_BUCKET_NUMBER)
            {
                m_bucketNumber = bucketNumber;
            }
        }
    }

    void setSyncResults(bool _syncResults) { m_enableSyncResults = _syncResults; }
    [[nodiscard]] bool enableSyncResults() const { return m_enableSyncResults; }

    void setBucketNumber(uint16_t _bucketNumber) { m_bucketNumber = _bucketNumber; }
    [[nodiscard]] uint16_t bucketNumber() const { return m_bucketNumber; }

    void setSeed(bcos::bytes _seed) { m_seed = std::move(_seed); }
    [[nodiscard]] bcos::bytes const& seed() const { return m_seed; }

    void setLowBandwidth(bool _lowBandwidth) { m_lowBandwidth = _lowBandwidth; }
    [[nodiscard]] bool lowBandwidth() const { return m_lowBandwidth; }

private:
    // whether the results need to be synchronized to the counterparty
    bool m_enableSyncResults{false};

    // we can also run oprf with high performance just reducing the width of matrix
    // note that: it may cause oprf outputs conflict with very small probability
    bool m_enableHighPerformance{false};

    // one input will be mapped to multiple buckets
    // the number of buckets is also the width of matrix
    uint16_t m_bucketNumber{DEFAULT_BUCKET_NUMBER};

    bcos::bytes m_seed;

    bool m_lowBandwidth;
};
}  // namespace ppc::psi
