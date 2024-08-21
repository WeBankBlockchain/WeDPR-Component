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
 * @file error.cpp
 * @author: yujiechen
 * @date 2023-08-11
 */


#include "error.h"

#include <mutex>
#include <string>
#include <thread>

// record c error
thread_local int g_global_error = 0;
thread_local std::string g_global_msg;

/**
 * @brief the last sync operation success or not
 *
 * @return int
 */
int last_call_success()
{
    return g_global_error == PPC_CRYPTO_C_SDK_SUCCESS;
}

/**
 * @brief gets status of the most recent sync operation
 *
 * @return int
 */
int get_last_error()
{
    return g_global_error;
}

/**
 * @brief gets error message of the most recent sync operation, effect if get_last_error
 * return not zero
 *
 * @return const char*
 */
const char* get_last_error_msg()
{
    return g_global_msg.c_str();
}

void clear_last_error()
{
    g_global_error = 0;
    g_global_msg.clear();
}

void set_last_error_msg(int error, const char* msg)
{
    g_global_error = error;
    g_global_msg = std::string(msg);
}