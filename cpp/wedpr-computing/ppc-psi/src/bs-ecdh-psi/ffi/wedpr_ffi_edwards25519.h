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
* @file wedpr_ffi_edwards25519.h
* @author: shawnhe
* @date 2023-10-10
 */


#ifndef _WEDPR_FFI_EDWARDS25519_H_
#define _WEDPR_FFI_EDWARDS25519_H_

#include <cstdarg>
#include <cstdint>
#include <cstdlib>

#include "wedpr_utilities.h"

extern "C" {
/**
 * C interface for 'generate random scalar'.
 */
int8_t wedpr_random_scalar(COutputBuffer* random);

/**
 * C interface for 'hash to curve'.
 */
int8_t wedpr_hash_to_curve(const CInputBuffer* message, COutputBuffer* point);

/**
 * C interface for 'point multiply by scalar'.
 */
int8_t wedpr_point_scalar_multi(
    const CInputBuffer* raw_point, const CInputBuffer* scalar, COutputBuffer* point);
}

#endif
