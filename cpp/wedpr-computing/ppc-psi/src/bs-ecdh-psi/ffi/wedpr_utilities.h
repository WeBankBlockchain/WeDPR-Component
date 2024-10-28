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
 * @file wedpr_utilities.h
 * @author: shawnhe
 * @date 2023-10-10
 */

#ifndef _WEDPR_UTILITIES_H_
#define _WEDPR_UTILITIES_H_

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <ostream>

#define POINT_SIZE 32
#define SCALAR_SIZE 32

extern "C" {

struct CInputBuffer
{
    const char* data;
    uintptr_t len;
};

struct COutputBuffer
{
    char* data;
    uintptr_t len;
};

const int8_t WEDPR_ERROR = -1;
const int8_t WEDPR_SUCCESS = 0;
}  // extern "C"

#endif
