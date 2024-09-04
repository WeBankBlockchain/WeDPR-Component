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
 * @date 2022-10-20
 */
#pragma once

#include "ppc-framework/Common.h"
#include "ppc-framework/storage/FileStorage.h"
#include <bcos-utilities/Log.h>
#include <hdfs/hdfs.h>

#define IO_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("IO")

namespace ppc::io
{
DERIVE_PPC_EXCEPTION(ConnectionOptionNotFound);
DERIVE_PPC_EXCEPTION(InvalidMmapGranularity);
DERIVE_PPC_EXCEPTION(MmapFileException);
DERIVE_PPC_EXCEPTION(InvalidParam);
DERIVE_PPC_EXCEPTION(OpenFileLineWriterException);
DERIVE_PPC_EXCEPTION(CloseFileLineWriterException);
DERIVE_PPC_EXCEPTION(UnSupportedDataResource);
DERIVE_PPC_EXCEPTION(LoadDataResourceException);

DERIVE_PPC_EXCEPTION(HDFSOpenMetaInfoFailed);
DERIVE_PPC_EXCEPTION(HDFSReadDataFailed);
DERIVE_PPC_EXCEPTION(HDFSWriteDataFailed);
DERIVE_PPC_EXCEPTION(HDFSFlushFailed);

struct HdfsFileWrapper
{
    HdfsFileWrapper(ppc::storage::FileHandler::Ptr _handler, HdfsFileInternalWrapper* _fs)
      : handler(std::move(_handler)), fs(_fs)
    {}
    ppc::storage::FileHandler::Ptr handler;
    HdfsFileInternalWrapper* fs;
};

struct HDFSFileDeleter
{
    void operator()(HdfsFileWrapper* _wrapper) const
    {
        if (!_wrapper->fs)
        {
            return;
        }
        hdfsCloseFile((hdfsFS)_wrapper->handler->storageHandler(), _wrapper->fs);
    }
    ppc::storage::FileHandler::Ptr handler;
};
}  // namespace ppc::io