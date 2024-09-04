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
 * @file Common.h
 * @author: shawnhe
 * @date 2023-09-22
 */

#pragma once

#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/PPCMessageFace.h"

#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>


namespace ppc::psi
{
DERIVE_PPC_EXCEPTION(BsEcdhException);

#define BS_ECDH_PSI_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PSI: BS-ECDH-PSI")
#define INDEX_FILE_SUFFIX ".index"
#define AUDIT_FILE_SUFFIX ".evidence"
#define MAX_TASK_COUNT 32
#define PAUSE_THRESHOLD 60000

}  // namespace ppc::psi
