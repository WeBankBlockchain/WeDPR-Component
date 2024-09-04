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
 * @file paillier_enc.h
 * @author: caryliao
 * @date 2023-12-4
 */

#ifndef __PAILLIER_DECRYPT_H__
#define __PAILLIER_DECRYPT_H__

#include <mysql.h>
#include <mysql_com.h>

#ifdef __cplusplus
extern "C" {
#endif
my_bool paillier_enc_init(UDF_INIT* initid, UDF_ARGS* args, char* message);

void paillier_enc_deinit(UDF_INIT* initid);

// Note: every row data will trigger this function
char* paillier_enc(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length,
    char* is_null, char* error);

#ifdef __cplusplus
}
#endif
#endif
