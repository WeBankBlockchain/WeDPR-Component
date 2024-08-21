/**
 *  Copyright (C) 2023 WeDPR.
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
 * @file utilities.cpp
 * @author: yujiechen
 * @date 2023-08-22
 */
#include "utilities.h"
#include "error.h"
#include <bcos-utilities/DataConvertUtility.h>

using namespace bcos;

int max_msg_size = 255;

void to_hex(OutputBuffer* hexData, InputBuffer const* bytesData)
{
    clear_last_error();
    try
    {
        auto hexStr = *(bcos::toHexString(bytesData->data, bytesData->data + bytesData->len));
        hexData->len = hexStr.size();
        memcpy((void*)hexData->data, (const void*)hexStr.data(), hexData->len);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        BCOS_LOG(ERROR) << LOG_DESC("to_hex error") << errorMsg;
        set_last_error_msg(-1, errorMsg.data());
    }
}

void from_hex(OutputBuffer* bytesData, InputBuffer const* hexData)
{
    clear_last_error();
    try
    {
        // TODO: decrease the copy overhead
        auto hexString = std::string(hexData->data, hexData->data + hexData->len);
        auto bytesResult = *(bcos::fromHexString(hexString));
        bytesData->len = bytesResult.size();
        memcpy((void*)bytesData->data, (const void*)bytesResult.data(), bytesResult.size());
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        BCOS_LOG(ERROR) << LOG_DESC("from_hex error") << errorMsg;
        set_last_error_msg(-1, errorMsg.data());
    }
}