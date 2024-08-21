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
 * @file buffer.h
 * @author: yujiechen
 * @date 2023-08-11
 */
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
    const unsigned char* data;
    uint64_t len;
} InputBuffer;

typedef struct
{
    unsigned char* data;
    uint64_t len;
} OutputBuffer;

#ifdef __cplusplus
}
#endif
#endif
