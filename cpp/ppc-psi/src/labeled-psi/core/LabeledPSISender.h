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
 * @file LabeledPSISender.h
 * @author: shawnhe
 * @date 2022-11-6
 */


#pragma once

#include <memory>

#include <apsi/powers.h>
#include <apsi/psi_params.h>
#include <apsi/seal_object.h>
#include <seal/seal.h>

#include "ResultPackage.h"
#include "SenderDB.h"
#include "ppc-framework/crypto/Oprf.h"
#include "ppc-framework/protocol/Task.h"
#include "ppc-front/ppc-front/PPCChannel.h"
#include "ppc-psi/src/labeled-psi/LabeledPSIConfig.h"
#include "ppc-psi/src/labeled-psi/protocol/Protocol.h"
#include "protocol/src/PPCMessage.h"

namespace ppc::psi
{
class LabeledPSISender : public std::enable_shared_from_this<LabeledPSISender>
{
public:
    using Ptr = std::shared_ptr<LabeledPSISender>;
    virtual ~LabeledPSISender() = default;

    LabeledPSISender(LabeledPSIConfig::Ptr _config,
        std::function<void(const std::string&, bcos::Error::Ptr)> _taskEndTrigger);

public:
    // send psi params for some task
    void handlePsiParamsRequest(const front::PPCMessageFace::Ptr& _message);

    // receive blinded items and send evaluated items
    void handleBlindedItems(const front::PPCMessageFace::Ptr& _message);

    // receive encrypted powers and send polynomial ciphertext response
    void handleQuery(const front::PPCMessageFace::Ptr& _message);

    void setSenderDB(SenderDB::Ptr _senderDB) { m_senderDB = std::move(_senderDB); }

protected:
    void computePowers(const std::string& _taskID, const std::string& _receiverID,
        const apsi::CryptoContext& _cryptoContext,
        std::shared_ptr<std::vector<std::vector<seal::Ciphertext>>> _allPowers,
        std::shared_ptr<seal::MemoryPoolHandle> _memoryPool, const apsi::PowersDag& _powersDag,
        uint32_t _bundleIdx);
    void processBinBundleCache(const std::string& _taskID, const std::string& _receiverID,
        const apsi::CryptoContext& _cryptoContext,
        const std::shared_ptr<std::vector<std::vector<seal::Ciphertext>>>& _allPowers,
        const std::shared_ptr<seal::MemoryPoolHandle>& _memoryPool,
        std::reference_wrapper<const BinBundleCache> _cache, uint32_t _bundleIdx, uint32_t _seq);
    void sendResultPackage(const std::string& _taskID, const std::string& _receiverID,
        const ResultPackage& _resultPackage, uint32_t _seq);

    void onSenderException(
        const std::string& _taskID, const std::string& _module, const std::exception& _e);
    void onSenderTaskDone(const std::string& _taskID, bcos::Error::Ptr _error);

private:
    LabeledPSIConfig::Ptr m_config;

    std::function<void(const std::string&, bcos::Error::Ptr)> m_taskEndTrigger;

    front::PPCMessageFactory::Ptr m_messageFactory;

    // set after initialization of sender
    SenderDB::Ptr m_senderDB;

    seal::compr_mode_type m_comprMode{seal::Serialization::compr_mode_default};
};
}  // namespace ppc::psi
