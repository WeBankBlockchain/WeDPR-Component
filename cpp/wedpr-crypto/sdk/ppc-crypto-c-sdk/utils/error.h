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
 * @file error.h
 * @author: yujiechen
 * @date 2023-08-11
 */

#ifndef __INCLUDE_ERROR__
#define __INCLUDE_ERROR__

#ifdef __cplusplus
extern "C" {
#endif

#define PPC_CRYPTO_C_SDK_SUCCESS (0)

/**
 * @brief the last sync operation success or not
 *
 * @return int
 */
int last_call_success();

/**
 * @brief gets status of the recent sync operation
 *  Note: thread safe operation
 *
 * @return int
 */
int get_last_error();

/**
 * @brief gets error message of the recent sync operation, effect if get_last_error,
 * return not zero
 * Note: thread safe operation
 *
 * @return const char*
 */
const char* get_last_error_msg();

/**
 * @brief clear the last error
 */
void clear_last_error();

/**
 * @brief set the last error and error message
 */
void set_last_error_msg(int error, const char* msg);

#ifdef __cplusplus
}
#endif
#endif
