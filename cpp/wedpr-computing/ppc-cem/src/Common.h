/*
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
 * @author: caryliao
 * @date 2022-11-03
 */
#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/Log.h>

#define CEM_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("CM")

namespace ppc::cem
{
const size_t CIPHERTEXT_LEN = 144;
DERIVE_PPC_EXCEPTION(OpenDatasetFileFailException);
DERIVE_PPC_EXCEPTION(CipherMatchFailException);
}  // namespace ppc::cem