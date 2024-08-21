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
 * @file EcdhConnPSIClient.h
 * @author: zachma
 * @date 2023-7-18
 */

#pragma once
#include "../../psi-framework/TaskState.h"
#include "../EcdhConnPSIConfig.h"
#include "../EcdhConnTaskState.h"
#include "../protocol/PSIConnMessageFactory.h"
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Exceptions.h>

namespace ppc::psi
{
class EcdhConnPSIClient : public std::enable_shared_from_this<EcdhConnPSIClient>
{
public:
    using Ptr = std::shared_ptr<EcdhConnPSIClient>;
    EcdhConnPSIClient(EcdhConnPSIConfig::Ptr _config, EcdhConnTaskState::Ptr _taskState);
    virtual ~EcdhConnPSIClient() = default;
    const std::string& taskID() const { return m_taskID; }


    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);

    virtual void sendHandshakeRequest(TaskState::Ptr _taskState);
    virtual void onSecondCipherServerToClient(const bcos::bytes& _msg);
    virtual void onCipherServerToClient(const bcos::bytes& _msg);
    virtual void onHandShakeResponseHandler(const bcos::bytes& _msg);

protected:
    virtual void computeAndEncryptSet(bcos::bytesPointer _randA);
    virtual void computeAndEncryptSetSecond(
        const std::vector<bcos::bytes>& _input, bcos::bytesPointer _randA);
    virtual std::vector<bcos::bytes> tryIntersection(const std::vector<bcos::bytes>& _input1,
        const std::vector<bcos::bytes>& _input2, int _compareSuffixBitSize = 12);

private:
    int m_send_count = 0;
    ppc::io::DataBatch::Ptr m_originInputs;
    PSIConnMessageFactory::Ptr m_connMsgFactory;
    EcdhConnPSIConfig::Ptr m_config;
    std::string m_taskID;
    EcdhConnTaskState::Ptr m_taskState;
    PSIConnMessageFactory::Ptr m_msgFactory;

    std::map<std::string, std::string> m_header;
    // 保存最终结果
    bcos::bytes m_randomA;

    std::vector<bcos::bytes> m_Q2;
    std::vector<bcos::bytes> m_Q1;
    std::vector<bcos::bytes> m_P2_Client;
    std::vector<bcos::bytes> m_P1_Client;

private:
    virtual std::map<std::string, std::string> initHeader(const std::string& _taskId);
    virtual std::string generateKey(int _mainProcess, int _subProcess);
};
}  // namespace ppc::psi
