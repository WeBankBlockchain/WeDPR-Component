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
 * @file FrontServiceServer.cpp
 * @author: shawnhe
 * @date 2022-10-20
 */

#include "FrontServiceServer.h"
#include "ppc-framework/Common.h"
#include "ppc-tars-protocol/ppc-tars-protocol/Common.h"

#define FRONTSERVER_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("FrontServer")

using namespace ppctars;

ppctars::Error FrontServiceServer::onReceiveMessage(
    const vector<tars::Char>& _message, tars::TarsCurrentPtr _current)
{
    FRONTSERVER_LOG(TRACE) << LOG_DESC("receive message from gateway");

    auto startT = bcos::utcSteadyTime();
    _current->setResponse(false);

    auto ppcMessage = m_frontInitializer->messageFactory()->buildPPCMessage(
        bcos::bytesConstRef((bcos::byte const*)_message.data(), _message.size()));
    if (!ppcMessage)
    {
        auto error = ppctars::Error();
        error.errorCode = ppc::protocol::PPCRetCode::DECODE_PPC_MESSAGE_ERROR;
        error.errorMessage = "decode ppc message error";
        return error;
    }
    auto msgSize = _message.size();
    m_frontInitializer->front()->onReceiveMessage(
        ppcMessage, [_current, startT, msgSize](bcos::Error::Ptr error) {
            BCOS_LOG(TRACE) << LOG_DESC("onReceiveMessage") << LOG_KV("msgSize", msgSize)
                            << LOG_KV("timecost", (bcos::utcSteadyTime() - startT));
            async_response_onReceiveMessage(_current, toTarsError(error));
        });

    return {};
}