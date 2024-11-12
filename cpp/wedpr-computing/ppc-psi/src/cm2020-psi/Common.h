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
 * @file Common.h
 * @author: shawnhe
 * @date 2022-12-7
 */
#pragma once

#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/PPCMessageFace.h"
#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>
#include <tbb/tbb.h>


namespace ppc::psi
{
DERIVE_PPC_EXCEPTION(CM2020Exception);

#define CM2020_PSI_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PSI: CM2020-PSI")
#define HIGH_PERFORMANCE_BUCKET_NUMBER (256)
#define DEFAULT_BUCKET_NUMBER (320)
#define MAX_BUCKET_NUMBER (400)
#define DEFAULT_HANDLE_WIDTH_POWER (24)
#define MIN_HANDLE_WIDTH (8)
#define MAX_SEND_BUFFER_LENGTH (8 * 1024 * 1024)
#define MIN_BUCKET_SIZE (8 * 1024)
#define MAX_COMPUTE_HASH_SIZE (1 << 26)
#define ENCODE_RATE (2)
#define RESULT_LEN_BYTE (16)
#define WAITING_PEER_FINISH_M (10)

enum class CM2020PSIMessageType : uint8_t
{
    HELLO_RECEIVER = 0x01,
    HELLO_SENDER = 0x02,
    RECEIVER_SIZE = 0x03,
    SENDER_SIZE = 0x04,
    POINT_A = 0x05,
    POINT_B_ARRAY = 0x06,
    MATRIX = 0x07,
    DO_NEXT_ROUND = 0x08,
    HASHES = 0x09,
    RESULTS_SIZE = 0x0a,
    RESULTS = 0x0b
};

enum class CM2020PSIRetCode : int
{
    ON_EXCEPTION = -3000,
    UNDEFINED_TASK_ROLE = -3001,
    INVALID_TASK_PARAM = -3002
};

inline uint32_t dedupDataBatch(ppc::io::DataBatch::Ptr dataBatch)
{
    if (!dataBatch || dataBatch->mutableData() == nullptr || dataBatch->mutableData()->empty())
    {
        return 0;
    }
    auto& data = dataBatch->mutableData();
    // Note: the header field should not been sorted
    auto it = data->begin() + 1;
    if (it >= data->end())
    {
        return data->size();
    }
    tbb::parallel_sort(it, data->end());
    auto unique_end = std::unique(it, data->end());
    data->erase(unique_end, data->end());
    return data->size();
}

}  // namespace ppc::psi
