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
 * @author: yujiechen
 * @date 2022-10-25
 */
#pragma once
#include "../Common.h"
#define MYSQL_STORAGE_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("MYSQL_STORAGE")

namespace ppc::storage
{
DERIVE_PPC_EXCEPTION(ConnectMySQLError);
DERIVE_PPC_EXCEPTION(ExecMySQLError);
DERIVE_PPC_EXCEPTION(ParseMySQLResultError);
DERIVE_PPC_EXCEPTION(UnSupportedDataSchema);
DERIVE_PPC_EXCEPTION(NotSetMySQLConnectionOption);
}  // namespace ppc::storage