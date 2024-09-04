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
 * @file Common.h
 * @author: shawnhe
 * @date 2022-11-4
 */
#pragma once

#include "ppc-framework/Common.h"
#include "ppc-framework/protocol/PPCMessageFace.h"

#include <bcos-utilities/Common.h>
#include <bcos-utilities/Log.h>


namespace ppc::psi
{
DERIVE_PPC_EXCEPTION(ConfigPowersDagException);
DERIVE_PPC_EXCEPTION(TooManyItemsException);
DERIVE_PPC_EXCEPTION(ResultPackageException);
DERIVE_PPC_EXCEPTION(OverwriteItemException);
DERIVE_PPC_EXCEPTION(InsertItemException);
DERIVE_PPC_EXCEPTION(RemoveItemException);
DERIVE_PPC_EXCEPTION(LabelExceedsSizeException);
DERIVE_PPC_EXCEPTION(NonceExceedsSizeException);
DERIVE_PPC_EXCEPTION(RetrieveItemException);
DERIVE_PPC_EXCEPTION(RetrieveLabelException);
DERIVE_PPC_EXCEPTION(ExtracteRelinKeysException);
DERIVE_PPC_EXCEPTION(ExtracteCiphertextException);
DERIVE_PPC_EXCEPTION(QueryPowersException);
DERIVE_PPC_EXCEPTION(PowerDagException);

#define LABELED_PSI_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PSI: Labeled-PSI")

#define CUCKOO_TABLE_INSERT_ATTEMPTS 500
#define MAX_LABEL_BYTE 1024
#define MAX_QUERY_SIZE 8

constexpr static std::string_view CMD_SETUP_SENDER_DB = "setup_sender_db";
constexpr static std::string_view CMD_RUN_LABELED_PSI = "run_labeled_psi";
constexpr static std::string_view CMD_SAVE_SENDER_CACHE = "save_sender_cache";
constexpr static std::string_view CMD_LOAD_SENDER_CACHE = "load_sender_cache";
constexpr static std::string_view CMD_ADD_ITEMS = "add_items";
constexpr static std::string_view CMD_UPDATE_LABELS = "update_labels";
constexpr static std::string_view CMD_DELETE_ITEMS = "delete_items";

enum class LabeledPSIMessageType : uint8_t
{
    PARAMS_REQUEST = 0x01,
    PARAMS_RESPONSE = 0x02,
    OPRF_BLINDED_ITEMS = 0x03,
    OPRF_EVALUATED_ITEMS = 0x04,
    QUERY = 0x05,
    RESPONSE = 0x06
};

enum class LabeledPSIRetCode : int
{
    ON_EXCEPTION = -2000,
    SENDER_NOT_READY = -2001,
    CUCKOO_HASH_ERROR = -2002,
    MISSING_LABEL_DATA = -2003,
    LACK_LABEL_DATA = -2004,
    UNDEFINED_TASK_ROLE = -2005,
    UNDEFINED_COMMAND = -2006,
    DATA_FORMAT_ERROR = -2007,
    INVALID_TASK_PARAM = -2008,
    SETUP_SENDER_ERROR = -2009,
    LOAD_SENDER_CACHE_ERROR = -2010
};

enum class LabeledPSICommand : int
{
    SETUP_SENDER_DB,
    RUN_LABELED_PSI,
    SAVE_SENDER_CACHE,
    LOAD_SENDER_CACHE,
    ADD_ITEMS,
    UPDATE_LABELS,
    DELETE_ITEMS
};

template <typename T>
bool hasNZeros(T* ptr, size_t count)
{
    return std::all_of(ptr, ptr + count, [](auto a) { return a == T(0); });
}

}  // namespace ppc::psi
