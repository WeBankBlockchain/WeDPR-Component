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
 * @file MessageImpl.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#include "P2PMessageImpl.h"
#include "../Common.h"

using namespace bcos;
using namespace ppc::protocol;


bool P2PMessageImpl::encode(bcos::boostssl::EncodedMsg& encodedMsg)
{
    try
    {
        // header
        m_msg->header()->encode(encodedMsg.header);
        // assign the payload back
        encodedMsg.payload = m_msg->payload();
        return true;
    }
    catch (std::exception const& e)
    {
        GATEWAY_LOG(WARNING) << LOG_DESC("encode message failed")
                             << LOG_KV("error", boost::diagnostic_information(e));
        return false;
    }
}