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
 * @date 2022-11-30
 */
#pragma once
#include "../Common.h"
#define HDFS_STORAGE_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("HDFS_STORAGE")
#define HDFS_AUTH_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("HDFS_STORAGE_AUTH")
namespace ppc::storage
{
DERIVE_PPC_EXCEPTION(ConnectToHDFSFailed);
DERIVE_PPC_EXCEPTION(CreateDirectoryFailed);
DERIVE_PPC_EXCEPTION(UnsupportedFileStorage);
DERIVE_PPC_EXCEPTION(DeleteHDFSFileFailed);
DERIVE_PPC_EXCEPTION(HDFSConnectionOptionNotSet);
DERIVE_PPC_EXCEPTION(RenameHDFSFileFailed);
}  // namespace ppc::storage