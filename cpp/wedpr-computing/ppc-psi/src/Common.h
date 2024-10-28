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
 * @date 2022-12-26
 */
#pragma once
#include <cstdint>
#include <sstream>
#define PSI_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("PSI")

namespace ppc::psi
{
enum class CommonMessageType : uint8_t
{
    ErrorNotification = 0xff,
    PingPeer = 0xfe,
};

// the common psi packet-type
enum class PSIPacketType : uint32_t
{
    CancelTaskNotification = 0x01,  // notify the peer that the task has been canceled for some
                                    // reason
    TaskSyncMsg = 0x02,             // sync the task information
    HandshakeRequest = 0x03,        // the handshake request
    HandshakeResponse = 0x04,       // the handshake response
    PSIResultSyncMsg = 0x05,        // sync the psi-result to the psi-server in some cases
    PSIResultSyncResponse = 0x06,   // response for psi-result-sync
    HandshakeSuccess = 0x07,        // the client response to the server when handshake-success
};

// packetType for ra2018-psi
enum class RA2018PacketType : uint32_t
{
    CuckooFilterRequest = 100,         // the client request the cuckoofilter
    CuckooFilterResponse = 101,        // the server response the cuckoofilter
    EvaluateRequest = 102,             // the client request the server to evaluate-data
    EvaluateResponse = 103,            // the server response the evaluated-data
    MissingCuckooFilterRequest = 104,  // request the missed cuckoo-filter
    MissingCuckooFilterResponse,       // response to the missed-cuckoo-filter-request
};

// packetType for ecdh-psi
enum class ECDHPacketType : uint32_t
{
    EvaluateRequest = 100,
    EvaluateResponse = 101,
    ServerBlindedData = 102,
    SyncDataBatchInfo = 103,
};

enum PSIRetCode : int
{
    Success = 0,
    TaskParamsError = -1000,
    PeerNodeDown = -1001,
    DuplicatedTask = -1002,
    LoadDataFailed = -1003,
    PeerNotifyFinish = -1004,
    InvalidTaskParamForRA2018 = -1005,
    DataResourceOccupied = -1006,
    UnsupportedCommand = -1007,
    LoadCuckooFilterDataError = -1008,
    UnknownPSIPacketType = -1009,
    TaskNotFound = -1010,
    TaskInProcessing = -1011,
    OnlySupportOnePeer = -1012,
    UnsupportedPartyType = -1013,
    NotSpecifyInputDataResource = -1014,
    NotSpecifyOutputDataResource = -1015,
    NotOfflineFullEvaluated = -1016,
    BlindDataError = -1017,
    syncCuckooFilterError = -1018,
    NotSpecifyPeerDataResource = -1019,
    HandshakeFailed = -1020,
    SyncPSIResultFailed = -1021,
    RA2018PSIDisabled = -1022,
    HandleTaskError = -1023,
    TaskNotReady = -1024,
    TaskIsNotRunning = -1025,
    OnException = -1026,
    TaskExists = -1027,
    TaskKilled = -1028,
    TaskCountReachMax = -1029,
    TaskTimeout = -1030
};

enum CacheState
{
    Evaluating = 0,
    Finalizing = 1,
    Finalized = 2,
    IntersectionProgressing = 3,
    Intersectioned = 4,
    StoreProgressing = 5,
    Stored = 6,
    Syncing = 7,
    Synced = 8,
};

inline std::ostream& operator<<(std::ostream& _out, CacheState const& _state)
{
    switch (_state)
    {
    case CacheState::Evaluating:
        _out << "Evaluating";
        break;
    case CacheState::Finalizing:
        _out << "Finalizing";
        break;
    case CacheState::Finalized:
        _out << "Finalized";
        break;
    case CacheState::IntersectionProgressing:
        _out << "IntersectionProgressing";
        break;
    case CacheState::Intersectioned:
        _out << "Intersectioned";
        break;
    case CacheState::StoreProgressing:
        _out << "StoreProgressing";
        break;
    case CacheState::Stored:
        _out << "Stored";
        break;
    case CacheState::Syncing:
        _out << "Syncing";
        break;
    case CacheState::Synced:
        _out << "Synced";
        break;
    default:
        _out << "UnknownCacheState";
        break;
    }
    return _out;
}
}  // namespace ppc::psi
