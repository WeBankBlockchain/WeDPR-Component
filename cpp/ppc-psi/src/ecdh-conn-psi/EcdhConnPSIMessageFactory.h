/*
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
 * @file EcdhConnPSIMessageFactory.h
 * @author: zachma
 * @date 2023-8-23
 */

#pragma once
#include "Common.h"
#include "protocol/PSIConnMessage.h"
#include "protocol/transport.pb.h"

namespace ppc::psi
{
class EcdhConnPSIMessageFactory
{
public:
    using Ptr = std::shared_ptr<EcdhConnPSIMessageFactory>;
    EcdhConnPSIMessageFactory() = default;
    ~EcdhConnPSIMessageFactory() = default;

    PSIConnMessage::Ptr decodePSIConnMessage(bcos::bytes _data)
    {
        std::string _msgStr = "";
        _msgStr.assign(_data.begin(), _data.end());

        // parse PushRequest
        org::interconnection::link::PushRequest* pushRequest =
            new org::interconnection::link::PushRequest();
        pushRequest->ParseFromString(_msgStr);
        auto key = pushRequest->key();
        int sender = 0;
        int mainProcess = 0;
        int subProcess = -1;
        int receiver = 0;
        if (parseKey(key, &sender, &receiver, &mainProcess, &subProcess))
        {
            auto value = pushRequest->value();

            ECDH_CONN_LOG(INFO) << LOG_DESC("decodePSIConnMessage start")
                                << LOG_KV("sender", sender) << LOG_KV("receiver", receiver)
                                << LOG_KV("mainProcess", mainProcess)
                                << LOG_KV("subProcess", subProcess)
                                << LOG_KV("value: ", value.size());
            auto _psiConn = std::make_shared<PSIConnMessage>(key);
            _psiConn->setValue(bcos::bytes(value.begin(), value.end()));
            _psiConn->setSender(sender);
            _psiConn->setReceiver(receiver);
            _psiConn->setMainProcess(mainProcess);
            _psiConn->setSubProcess(subProcess);
            return _psiConn;
        }

        return nullptr;
    }

protected:
    // main: root:P2P-1:1->0{'\x01', '\x02'}[1]
    // sub: root-0:P2P-1:1->0{'\x01', '\x02'}[1]
    bool parseKey(
        const std::string& _key, int* _from, int* _to, int* _mainProcess, int* _subProcess)
    {
        if (NULL == _to || NULL == _mainProcess || NULL == _subProcess || NULL == _from)
        {
            return false;
        }

        std::vector<std::string> fieldNames;
        boost::split(fieldNames, _key, boost::is_any_of(":"));
        if (fieldNames.size() < 3)
        {
            return false;
        }
        // root-0
        std::string _sub_process_p2p = fieldNames.at(0);
        *_subProcess = _sub_process_p2p.back() - 48;
        if (*_subProcess < 0 || *_subProcess > 9)
        {
            *_subProcess = -1;
        }
        // P2P-1
        std::string _process_p2p = fieldNames.at(1);
        *_mainProcess = _process_p2p.back() - 48;

        // 1->0{'\x01', '\x02'}[1]
        std::string _to_p2p = fieldNames.at(2);
        std::vector<std::string> fieldNames2;
        boost::split(fieldNames2, _to_p2p, boost::is_any_of(std::string{'\x01', '\x02'}));
        // *_to = fieldNames2.at(0).back() - 48;
        // *_from = fieldNames2.at(0).front() - 48;
        *_to = 49 - fieldNames2.at(0).back();
        *_from = 49 - fieldNames2.at(0).front();
        return true;
    }
};
}  // namespace ppc::psi