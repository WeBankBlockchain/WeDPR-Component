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
 * @file LabeledPSIReceiver.h
 * @author: shawnhe
 * @date 2022-11-3
 */

#pragma once
#include "../../psi-framework/TaskState.h"
#include "ppc-framework/crypto/Oprf.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-framework/task/TaskFrameworkInterface.h"
#include "ppc-front/ppc-front/PPCChannel.h"
#include "ppc-psi/src/labeled-psi/LabeledPSIConfig.h"
#include "ppc-psi/src/labeled-psi/protocol/Protocol.h"
#include "ppc-tools/src/common/Progress.h"
#include "protocol/src/PPCMessage.h"

#include <apsi/crypto_context.h>
#include <apsi/itt.h>
#include <apsi/powers.h>
#include <apsi/psi_params.h>
#include <apsi/seal_object.h>
#include <gperftools/malloc_extension.h>
#include <memory>

#include <seal/seal.h>

namespace ppc::psi
{
class LabeledPSIReceiver : public std::enable_shared_from_this<LabeledPSIReceiver>
{
public:
    using Ptr = std::shared_ptr<LabeledPSIReceiver>;

    // construct after calling asyncRequestParams
    LabeledPSIReceiver(
        LabeledPSIConfig::Ptr _config, TaskState::Ptr _taskState, crypto::OprfClient::Ptr _client);

    ~LabeledPSIReceiver()
    {
        m_powersDag.reset();
        std::vector<std::string>().swap(m_items);
        std::pair<std::vector<apsi::HashedItem>, std::vector<apsi::LabelKey>>().swap(m_oprfOutputs);
        std::vector<kuku::item_type>().swap(m_kukuData);
        std::unordered_map<uint32_t, std::vector<apsi::SEALObject<seal::Ciphertext>>>().swap(
            m_encryptedPowers);
        std::vector<std::pair<std::string, std::string>>().swap(m_results);
        // release the memory to os
        MallocExtension::instance()->ReleaseFreeMemory();
        LABELED_PSI_LOG(INFO) << LOG_DESC("the sender destroyed") << LOG_KV("taskID", m_taskID);
    }

    void asyncRunTask();

    // init receiver by psi params
    void handlePsiParams(const front::PPCMessageFace::Ptr& _message);

    // finish oprf and run query
    void handleEvaluatedItems(const front::PPCMessageFace::Ptr& _message);

    // handle one response package
    void handleOneResponse(const front::PPCMessageFace::Ptr& _message);

    std::shared_ptr<seal::SEALContext> getSealContext() const
    {
        return m_cryptoContext.seal_context();
    }

    const std::string& taskID() const { return m_taskID; }

protected:
    void runReceiver();

    // init receiver
    void initializeByParams();

    // generates a new set of keys to use for queries
    void generateKeys();

    /**
     *Computes the PowersDag. The function returns the depth of the
     *PowersDag. In some cases the receiver may want to ensure that the depth of
     *the powers computation will be as expected (PowersDag::depth), and
     *otherwise attempt to reconfigure the PowersDag.
     */
    std::uint32_t computePowersDag(const std::set<std::uint32_t>& _sourcePowers);

    // run oprf as receiver
    void runOprfAsClient();
    void doOprfFinalize(const std::vector<bcos::bytes>& _evaluatedItems);

    // process oprf results and send query quest
    void requestQuery();
    void prepareQueryData();
    void doCuckooHashing();
    void computeEncryptedPower();

    // decrypt psi results and labels, handled by per bundle
    void decryptResults(const ppctars::QueryResponse& _queryResponse);

    void onReceiverException(const std::string& _module, const std::exception& e);
    void onReceiverTaskDone(bcos::Error::Ptr _error);

private:
    LabeledPSIConfig::Ptr m_config;
    TaskState::Ptr m_taskState;
    crypto::OprfClient::Ptr m_oprfClient;

    std::string m_taskID;

    std::shared_ptr<apsi::PSIParams> m_psiParams;

    apsi::CryptoContext m_cryptoContext;
    apsi::PowersDag m_powersDag;
    apsi::SEALObject<seal::RelinKeys> m_relinKeys;
    seal::compr_mode_type m_comprMode{seal::Serialization::compr_mode_default};

    std::vector<std::string> m_items;
    std::pair<std::vector<apsi::HashedItem>, std::vector<apsi::LabelKey>> m_oprfOutputs;
    apsi::receiver::IndexTranslationTable m_itt;
    std::vector<kuku::item_type> m_kukuData;
    std::unordered_map<uint32_t, std::vector<apsi::SEALObject<seal::Ciphertext>>> m_encryptedPowers;

    uint32_t m_responseCount{0};
    std::vector<std::pair<std::string, std::string>> m_results;

    tools::Progress::Ptr m_progress;
};
}  // namespace ppc::psi