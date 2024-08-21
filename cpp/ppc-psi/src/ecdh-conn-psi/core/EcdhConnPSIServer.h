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
 * @file EcdhConnPSIServer.h
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
class EcdhConnPSIServer : public std::enable_shared_from_this<EcdhConnPSIServer>
{
public:
    using Ptr = std::shared_ptr<EcdhConnPSIServer>;
    EcdhConnPSIServer(EcdhConnPSIConfig::Ptr _config, EcdhConnTaskState::Ptr _taskState);
    virtual ~EcdhConnPSIServer() = default;
    const std::string& taskID() const { return m_taskID; }

    virtual void asyncStartRunTask(ppc::protocol::Task::ConstPtr _task);

    virtual void onHandShakeRequestHandler(const bcos::bytes& _msg);
    virtual void onCipherClientToServer(const bcos::bytes& _msg);
    virtual void onSecondCipherClientToServer(const bcos::bytes& _msg);

protected:
    virtual void computeAndEncryptSet(bcos::bytesPointer _randB);
    virtual void computeAndEncryptSetSecond(
        const std::vector<bcos::bytes>& _input, bcos::bytesPointer _randB);
    virtual std::vector<bcos::bytes> tryIntersection(const std::vector<bcos::bytes>& _input1,
        const std::vector<bcos::bytes>& _input2, int _compareSuffixBitSize = 12);
    virtual int32_t selectCryptoAlgorithm(
        std::set<int32_t> _clientSupportedAlgorithms, std::set<int32_t> _localSupportedAlgorithms);

private:
    int m_send_count = 0;
    bcos::bytes m_randomB;
    ppc::io::DataBatch::Ptr m_originInputs;
    EcdhConnPSIConfig::Ptr m_config;
    std::string m_taskID;
    EcdhConnTaskState::Ptr m_taskState;
    PSIConnMessageFactory::Ptr m_msgFactory;

    std::map<std::string, std::string> m_header;


    // 保存最终结果
    std::vector<bcos::bytes> m_Q1_server;
    std::vector<bcos::bytes> m_Q2_server;

    std::vector<bcos::bytes> m_P2;
    std::vector<bcos::bytes> m_P1;

private:
    virtual std::string generateKey(int _mainProcess, int _subProcess);
    virtual std::map<std::string, std::string> initHeader(const std::string& _taskId);
};
}  // namespace ppc::psi
