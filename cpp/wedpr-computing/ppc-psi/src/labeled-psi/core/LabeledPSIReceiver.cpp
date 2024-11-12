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
 * @file LabeledPSIReceiver.cpp
 * @author: shawnhe
 * @date 2022-11-3
 */

#include <apsi/plaintext_powers.h>
#include <apsi/util/db_encoding.h>
#include <apsi/util/label_encryptor.h>
#include <apsi/util/utils.h>
#include <gsl/span>

#include "LabeledPSI.h"
#include "LabeledPSIReceiver.h"
#include "ResultPackage.h"
#include "bcos-utilities/DataConvertUtility.h"
#include "ppc-psi/src/labeled-psi/Common.h"
#include "ppc-psi/src/labeled-psi/protocol/LabeledPSIResult.h"
#include "wedpr-protocol/tars/TarsSerialize.h"

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::crypto;
using namespace ppc::tools;

LabeledPSIReceiver::LabeledPSIReceiver(
    LabeledPSIConfig::Ptr _config, TaskState::Ptr _taskState, OprfClient::Ptr _oprfClient)
  : m_config(std::move(_config)),
    m_taskState(std::move(_taskState)),
    m_oprfClient(std::move(_oprfClient))
{
    m_taskID = m_taskState->task()->id();
    auto originData = m_taskState->task()->selfParty()->dataResource()->rawData();
    if (originData.empty() || originData[0].size() > MAX_QUERY_SIZE)
    {
        BOOST_THROW_EXCEPTION(
            TooManyItemsException() << bcos::errinfo_comment(
                "Items count per query should be (0, " + std::to_string(MAX_QUERY_SIZE) + "]"));
    }
    m_items = std::move(originData[0]);
}

void LabeledPSIReceiver::asyncRunTask()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("asyncRunTask as receiver") << LOG_KV("taskID", m_taskID);
    m_config->threadPool()->enqueue([self = weak_from_this()]() {
        auto receiver = self.lock();
        if (!receiver)
        {
            return;
        }
        try
        {
            receiver->runReceiver();
        }
        catch (const std::exception& e)
        {
            receiver->onReceiverException("runTask", e);
        }
    });
}

void LabeledPSIReceiver::handlePsiParams(const front::PPCMessageFace::Ptr& _message)
{
    auto taskID = _message->taskID();
    LABELED_PSI_LOG(INFO) << LOG_BADGE("handlePsiParams") << LOG_KV("taskID", taskID);

    try
    {
        ppctars::PsiParams tarsParams;
        ppctars::serialize::decode(*_message->data(), tarsParams);
        m_psiParams = std::make_shared<apsi::PSIParams>(toPSIParams(tarsParams));
        m_responseCount = tarsParams.binBundleCount;

        m_progress = std::make_shared<Progress>(m_config->threadPool());
        m_progress->reset(m_responseCount, [self = weak_from_this()]() {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            receiver->onReceiverTaskDone(nullptr);
        });

        // initialize receiver by params
        initializeByParams();

        // do next step
        runOprfAsClient();
    }
    catch (const std::exception& e)
    {
        onReceiverException("handlePsiParams", e);
    }
}

void LabeledPSIReceiver::handleEvaluatedItems(const front::PPCMessageFace::Ptr& _message)
{
    auto taskID = _message->taskID();
    LABELED_PSI_LOG(INFO) << LOG_BADGE("handleEvaluatedItems") << LOG_KV("taskID", taskID);

    try
    {
        ppctars::OprfData evaluatedData;
        ppctars::serialize::decode(*_message->data(), evaluatedData);

        // finish oprf
        doOprfFinalize(evaluatedData.data);

        // do next step
        requestQuery();
    }
    catch (const std::exception& e)
    {
        onReceiverException("handleEvaluatedItems", e);
    }
}

void LabeledPSIReceiver::handleOneResponse(const front::PPCMessageFace::Ptr& _message)
{
    auto taskID = _message->taskID();
    LABELED_PSI_LOG(INFO) << LOG_BADGE("handleOneResponse") << LOG_KV("taskID", taskID);

    try
    {
        ppctars::QueryResponse queryResponse;
        ppctars::serialize::decode(*_message->data(), queryResponse);

        // handle response
        decryptResults(queryResponse);

        m_progress->mark<int64_t>(_message->seq());
    }
    catch (const std::exception& e)
    {
        onReceiverException("handleOneResponse", e);
    }
}

void LabeledPSIReceiver::runReceiver()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("request psi params from sender")
                          << LOG_KV("taskID", m_taskID) << LOG_KV("itemsSize", m_items.size());

    auto message = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(protocol::TaskType::PSI),
        uint8_t(protocol::TaskAlgorithmType::LABELED_PSI_2PC), m_taskID,
        std::make_shared<bcos::bytes>());
    message->setMessageType(uint8_t(LabeledPSIMessageType::PARAMS_REQUEST));

    // send request
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), message, m_config->networkTimeout(),
        [self = weak_from_this()](const bcos::Error::Ptr& _error) {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                receiver->onReceiverTaskDone(_error);
            }
        },
        nullptr);
}

void LabeledPSIReceiver::initializeByParams()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("receiver do initialize") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("psiParams", m_psiParams->to_string());

    // initialize the CryptoContext with a new SEALContext
    m_cryptoContext = apsi::CryptoContext(*m_psiParams);

    // create new keys
    generateKeys();

    // set up the PowersDag
    computePowersDag((*m_psiParams).query_params().query_powers);
}

void LabeledPSIReceiver::generateKeys()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("receiver generateKeys") << LOG_KV("taskID", m_taskID);

    // generate new keys
    seal::KeyGenerator generator(*getSealContext());

    // set the symmetric key, encryptor, and decryptor
    m_cryptoContext.set_secret(generator.secret_key());

    // create Serializable<RelinKeys> and move to m_relinKeys for storage
    m_relinKeys.clear();
    if (getSealContext()->using_keyswitching())
    {
        seal::Serializable<seal::RelinKeys> relinKeys(generator.create_relin_keys());
        m_relinKeys.set(std::move(relinKeys));
    }
}

std::uint32_t LabeledPSIReceiver::computePowersDag(const std::set<std::uint32_t>& _sourcePowers)
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("receiver do compute PowersDag")
                          << LOG_KV("taskID", m_taskID);
    // first compute the target powers
    std::set<uint32_t> targetPowers = apsi::util::create_powers_set(
        m_psiParams->query_params().ps_low_degree, m_psiParams->table_params().max_items_per_bin);

    // configure the PowersDag
    m_powersDag.configure(_sourcePowers, targetPowers);

    // check that the PowersDag is valid
    if (!m_powersDag.is_configured())
    {
        LABELED_PSI_LOG(ERROR) << LOG_DESC("failed to configure PowersDag")
                               << LOG_KV("sourcePowers", apsi::util::to_string(_sourcePowers))
                               << LOG_KV("targetPowers", apsi::util::to_string(targetPowers));

        BOOST_THROW_EXCEPTION(
            ConfigPowersDagException() << bcos::errinfo_comment("failed to configure PowersDag"));
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("finish computing PowersDag")
                          << LOG_KV("PowersDagDepth", m_powersDag.depth());
    return m_powersDag.depth();
}

void LabeledPSIReceiver::runOprfAsClient()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("run oprf as client with sender")
                          << LOG_KV("taskID", m_taskID) << LOG_KV("itemsSize", m_items.size());

    // blind items
    ppctars::OprfData oprfData;
    oprfData.data = m_oprfClient->blind(m_items);

    auto blindedMessage = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(TaskType::PSI),
        uint8_t(TaskAlgorithmType::LABELED_PSI_2PC), m_taskID, std::make_shared<bcos::bytes>());
    blindedMessage->setMessageType(uint8_t(LabeledPSIMessageType::OPRF_BLINDED_ITEMS));
    ppctars::serialize::encode(oprfData, *blindedMessage->data());

    // send blinded items to sender
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), blindedMessage, m_config->networkTimeout(),
        [self = weak_from_this()](const bcos::Error::Ptr& _error) {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                receiver->onReceiverTaskDone(_error);
            }
        },
        nullptr);
}

void LabeledPSIReceiver::doOprfFinalize(const std::vector<bcos::bytes>& _evaluatedItems)
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("do oprf finalize") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("itemsSize", m_items.size());

    auto output = m_oprfClient->finalize(m_items, _evaluatedItems);

    m_oprfOutputs.first.resize(m_items.size());
    m_oprfOutputs.second.resize(m_items.size());

    auto hashLen = m_oprfOutputs.first[0].value().size();
    auto keyLen = m_oprfOutputs.second[0].size();

    for (uint32_t idx = 0; idx < m_items.size(); ++idx)
    {
        std::memcpy(m_oprfOutputs.first[idx].value().data(), output[idx].data(), hashLen);
        std::memcpy(m_oprfOutputs.second[idx].data(), output[idx].data() + hashLen, keyLen);
    }
}

void LabeledPSIReceiver::prepareQueryData()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("prepare query data") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("itemsSize", m_items.size());
    doCuckooHashing();

    computeEncryptedPower();
}

void LabeledPSIReceiver::doCuckooHashing()
{
    kuku::KukuTable cuckoo(m_psiParams->table_params().table_size,  // Size of the hash table
        0,                                                          // Not using a stash
        m_psiParams->table_params().hash_func_count,                // Number of hash functions
        {0, 0},                        // Hardcoded { 0, 0 } as the seed
        CUCKOO_TABLE_INSERT_ATTEMPTS,  // The number of insertion attempts
        {0, 0});

    LABELED_PSI_LOG(INFO) << LOG_BADGE("runCuckooHash") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("tableSize", cuckoo.table_size());

    // Hash the data into a cuckoo hash table
    for (size_t idx = 0; idx < m_oprfOutputs.first.size(); ++idx)
    {
        const auto& item = m_oprfOutputs.first[idx];
        if (!cuckoo.insert(item.get_as<kuku::item_type>().front()))
        {
            // Insertion can fail for two reasons:
            //
            //     (1) The item was already in the table, in which case the
            //     "leftover item" is empty; (2) Cuckoo hashing failed due to too
            //     small table or too few hash functions.
            //
            // In case (1) simply move on to the next item and log this issue. Case
            // (2) is a critical issue so throw an exception.
            if (cuckoo.is_empty_item(cuckoo.leftover_item()))
            {
                LABELED_PSI_LOG(DEBUG) << LOG_DESC("skipping repeated insertion of items")
                                       << LOG_KV("index", idx) << LOG_KV("item", item.to_string());
            }
            else
            {
                LABELED_PSI_LOG(ERROR)
                    << LOG_DESC("failed to insert item into cuckoo table") << LOG_KV("index", idx)
                    << LOG_KV("item", item.to_string()) << LOG_KV("fill-rate", cuckoo.fill_rate());

                onReceiverTaskDone(
                    std::make_shared<bcos::Error>((int)LabeledPSIRetCode::CUCKOO_HASH_ERROR,
                        "failed to insert item into cuckoo table, index: " + std::to_string(idx)));
            }
        }
    }
    LABELED_PSI_LOG(INFO) << LOG_BADGE("finishCuckooHash") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("tableSize", cuckoo.table_size())
                          << LOG_KV("fill-rate", cuckoo.fill_rate());

    // once the table is filled, fill the table_idx_to_item_idx map
    m_itt.item_count_ = m_oprfOutputs.first.size();
    for (size_t idx = 0; idx < m_oprfOutputs.first.size(); ++idx)
    {
        auto item_loc = cuckoo.query(m_oprfOutputs.first[idx].get_as<kuku::item_type>().front());
        m_itt.table_idx_to_item_idx_[item_loc.location()] = idx;
    }

    m_kukuData = cuckoo.table();

    // clear hashed items
    std::vector<apsi::HashedItem>().swap(m_oprfOutputs.first);
    MallocExtension::instance()->ReleaseFreeMemory();
}

void LabeledPSIReceiver::computeEncryptedPower()
{
    LABELED_PSI_LOG(INFO) << LOG_DESC("compute encrypted powers for items")
                          << LOG_KV("taskID", m_taskID) << LOG_KV("itemsSize", m_items.size());

    // set up unencrypted query data
    std::vector<apsi::receiver::PlaintextPowers> plainPowers;
    for (uint32_t idx = 0; idx < m_psiParams->bundle_idx_count(); ++idx)
    {
        // first, find the items for this bundle index
        gsl::span<const kuku::item_type> bundleItems(
            m_kukuData.data() + idx * m_psiParams->items_per_bundle(),
            m_psiParams->items_per_bundle());

        std::vector<uint64_t> algItems;
        for (auto& item : bundleItems)
        {
            // now set up a BitstringView to this item
            gsl::span<const uint8_t> itemBytes(
                reinterpret_cast<const uint8_t*>(item.data()), sizeof(item));
            apsi::BitstringView<const uint8_t> itemBits(itemBytes, m_psiParams->item_bit_count());

            // create an algebraic item by breaking up the item into modulo plain_modulus parts
            std::vector<uint64_t> algItem = apsi::util::bits_to_field_elts(
                itemBits, m_psiParams->seal_params().plain_modulus());
            copy(algItem.cbegin(), algItem.cend(), back_inserter(algItems));
        }

        // now that we have the algebraized items for this bundle index, we create a PlaintextPowers
        // object that computes all necessary powers of the algebraized items.
        plainPowers.emplace_back(std::move(algItems), *m_psiParams, m_powersDag);
    }

    // the very last thing to do is encrypt the plainPowers and consolidate the matching
    // powers for different bundle indices
    LABELED_PSI_LOG(INFO) << LOG_DESC("start encrypt powers") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("bundleCount", m_psiParams->bundle_idx_count());

    for (uint32_t idx = 0; idx < m_psiParams->bundle_idx_count(); ++idx)
    {
        // encrypt the data for this power
        auto encryptedPowers(plainPowers[idx].encrypt(m_cryptoContext));

        // move the encrypted data to encrypted_powers
        for (auto& ep : encryptedPowers)
        {
            m_encryptedPowers[ep.first].emplace_back(std::move(ep.second));
        }
    }

    // clear kuku items
    std::vector<kuku::item_type>().swap(m_kukuData);
    MallocExtension::instance()->ReleaseFreeMemory();
}

void LabeledPSIReceiver::requestQuery()
{
    prepareQueryData();

    // construct tars struct for request
    std::vector<uint8_t> keys;
    if (getSealContext()->using_keyswitching())
    {
        keys.resize(m_relinKeys.save_size(m_comprMode));
    }
    m_relinKeys.save(keys, m_comprMode);

    ppctars::QueryRequest queryRequest;
    queryRequest.relinKeys = std::move(keys);

    for (const auto& ep : m_encryptedPowers)
    {
        ppctars::EncryptedPowers tarsEps;
        tarsEps.power = ep.first;
        for (const auto& ct : ep.second)
        {
            std::vector<uint8_t> cipher;
            cipher.resize(ct.save_size(m_comprMode));
            auto size = ct.save(cipher, m_comprMode);
            cipher.resize(size);
            tarsEps.ciphertexts.emplace_back(std::move(cipher));
        }
        queryRequest.encryptedPowers.emplace_back(std::move(tarsEps));
    }

    auto queryMessage = m_config->ppcMsgFactory()->buildPPCMessage(uint8_t(TaskType::PSI),
        uint8_t(TaskAlgorithmType::LABELED_PSI_2PC), m_taskID, std::make_shared<bcos::bytes>());
    queryMessage->setMessageType(uint8_t(LabeledPSIMessageType::QUERY));
    ppctars::serialize::encode(queryRequest, *queryMessage->data());

    LABELED_PSI_LOG(INFO) << LOG_DESC("send query request") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("dataSize", queryMessage->data()->size());

    // send request
    m_config->front()->asyncSendMessage(
        m_taskState->peerID(), queryMessage, m_config->networkTimeout(),
        [self = weak_from_this()](const bcos::Error::Ptr& _error) {
            auto receiver = self.lock();
            if (!receiver)
            {
                return;
            }
            if (_error && _error->errorCode())
            {
                receiver->onReceiverTaskDone(_error);
            }
        },
        nullptr);

    // clear encrypted powers
    std::unordered_map<uint32_t, std::vector<apsi::SEALObject<seal::Ciphertext>>>().swap(
        m_encryptedPowers);
    MallocExtension::instance()->ReleaseFreeMemory();

    m_results.resize(m_items.size());
}

void LabeledPSIReceiver::decryptResults(const ppctars::QueryResponse& _queryResult)
{
    size_t bundleIdx = _queryResult.bundleIdx;

    LABELED_PSI_LOG(DEBUG) << LOG_DESC("decryptResults") << LOG_KV("taskID", m_taskID)
                           << LOG_KV("index", bundleIdx);

    // construct ResultPackage
    ResultPackage resultPackage;
    resultPackage.bundleIdx = bundleIdx;
    resultPackage.labelByteCount = _queryResult.labelByteCount;
    resultPackage.nonceByteCount = _queryResult.nonceByteCount;

    gsl::span<const uint8_t> cipherSpan(
        reinterpret_cast<const uint8_t*>(_queryResult.ciphertext.data()),
        _queryResult.ciphertext.size());
    resultPackage.psiCipherResult.load(getSealContext(), cipherSpan);

    for (const auto& labelResult : _queryResult.labelResults)
    {
        gsl::span<const uint8_t> labelSpan(
            reinterpret_cast<const uint8_t*>(labelResult.data()), labelResult.size());
        apsi::SEALObject<seal::Ciphertext> cipher;
        cipher.load(getSealContext(), labelSpan);
        resultPackage.labelCipherResults.emplace_back(std::move(cipher));
    }

    // decrypt and decode the result; the result vector will have full batch size
    PlainResultPackage plainRp = resultPackage.extract(m_cryptoContext);

    // check if we are supposed to have label data present but don't have for some reason
    auto labelByteCount = seal::util::safe_cast<size_t>(plainRp.labelByteCount);
    if (labelByteCount && plainRp.labelResults.empty())
    {
        onReceiverTaskDone(std::make_shared<bcos::Error>(
            (int)LabeledPSIRetCode::MISSING_LABEL_DATA, "missing label data"));
        return;
    }

    // read the nonce byte count and compute the effective label byte count; set the nonce byte
    // count to zero if no label is expected anyway
    size_t nonceByteCount =
        labelByteCount ? seal::util::safe_cast<size_t>(plainRp.nonceByteCount) : 0;
    size_t effectiveLabelByteCount = seal::util::add_safe(nonceByteCount, labelByteCount);

    // how much label data did we actually receive?
    size_t receivedLabelBitCount = seal::util::mul_safe(
        seal::util::safe_cast<size_t>(m_psiParams->item_bit_count()), plainRp.labelResults.size());

    // compute the received label byte count and check that it is not less than what was expected
    size_t receivedLabelByteCount = receivedLabelBitCount / 8;

    if (receivedLabelByteCount < nonceByteCount)
    {
        onReceiverTaskDone(std::make_shared<bcos::Error>((int)LabeledPSIRetCode::LACK_LABEL_DATA,
            "nonce bytes, expected: " + std::to_string(nonceByteCount) +
                ", received: " + std::to_string(receivedLabelByteCount)));
        return;
    }
    else if (receivedLabelByteCount < effectiveLabelByteCount)
    {
        onReceiverTaskDone(std::make_shared<bcos::Error>((int)LabeledPSIRetCode::LACK_LABEL_DATA,
            "label bytes, expected: " + std::to_string(labelByteCount) +
                ", received: " + std::to_string(receivedLabelByteCount - nonceByteCount)));
        return;
    }

    auto itemsPerBundle = seal::util::safe_cast<size_t>(m_psiParams->items_per_bundle());
    size_t bundleStart =
        seal::util::mul_safe(seal::util::safe_cast<size_t>(bundleIdx), itemsPerBundle);

    LABELED_PSI_LOG(DEBUG) << LOG_DESC("decryptResults") << LOG_KV("taskID", m_taskID)
                           << LOG_KV("bundleStart", bundleStart) << LOG_KV("bundleIdx", bundleIdx);

    auto feltsPerItem = seal::util::safe_cast<size_t>(m_psiParams->item_params().felts_per_item);
    seal::util::StrideIter<const uint64_t*> plainRpIter(plainRp.psiResult.data(), feltsPerItem);

    seal::util::seal_for_each_n(
        seal::util::iter(plainRpIter, size_t(0)), itemsPerBundle, [&](auto&& I) {
            // find feltsPerItem consecutive zeros
            bool match = hasNZeros(std::get<0>(I).ptr(), feltsPerItem);
            if (!match)
            {
                return;
            }

            // compute the cuckoo table index for this item. then find the corresponding index
            // in the input items vector, so we know where to place the result.
            size_t tableIdx = seal::util::add_safe(size_t(std::get<1>(I)), bundleStart);
            auto itemIdx = m_itt.find_item_idx(tableIdx);

            if (match)
            {
                LABELED_PSI_LOG(DEBUG) << LOG_KV("match", match) << LOG_KV("tableIdx", tableIdx)
                                       << LOG_KV("itemIdx", itemIdx);
            }

            // ff this tableIdx doesn't match any itemIdx, ignore the result no matter what it is
            if (itemIdx == m_itt.item_count())
            {
                return;
            }

            apsi::Label label;
            if (labelByteCount)
            {
                // collect the entire label into this vector
                apsi::util::AlgLabel algLabel;

                size_t labelOffset = seal::util::mul_safe(size_t(std::get<1>(I)), feltsPerItem);
                for (auto& labelResult : plainRp.labelResults)
                {
                    gsl::span<apsi::util::felt_t> labelSpan(
                        labelResult.data() + labelOffset, feltsPerItem);
                    std::copy(labelSpan.begin(), labelSpan.end(), back_inserter(algLabel));
                }

                // create the label
                apsi::EncryptedLabel encryptedLabel = apsi::util::dealgebraize_label(
                    algLabel, receivedLabelBitCount, m_psiParams->seal_params().plain_modulus());

                // resize down to the effective byte count
                encryptedLabel.resize(effectiveLabelByteCount);

                // decrypt the label
                label = apsi::util::decrypt_label(
                    encryptedLabel, m_oprfOutputs.second[itemIdx], nonceByteCount);
            }

            m_results[itemIdx] =
                std::make_pair(m_items[itemIdx], std::string(label.begin(), label.end()));
        });
}

void LabeledPSIReceiver::onReceiverException(const std::string& _module, const std::exception& _e)
{
    LABELED_PSI_LOG(ERROR) << LOG_BADGE("onReceiverException") << LOG_KV("taskID", m_taskID)
                           << LOG_KV("module", _module)
                           << LOG_KV("error", boost::diagnostic_information(_e));
    auto error = std::make_shared<bcos::Error>((int)LabeledPSIRetCode::ON_EXCEPTION,
        "exception caught while do " + _module + ", message: " + boost::diagnostic_information(_e));
    onReceiverTaskDone(error);
}

void LabeledPSIReceiver::onReceiverTaskDone(bcos::Error::Ptr _error)
{
    if (m_taskState->taskDone())
    {
        return;
    }
    LABELED_PSI_LOG(INFO) << LOG_BADGE("onReceiverDone") << LOG_KV("taskID", m_taskID)
                          << LOG_KV("success", _error == nullptr);
    auto result = std::make_shared<LabeledPSIResult>(m_taskID);
    if (_error)
    {
        result->setError(std::move(_error));
    }
    else
    {
        std::vector<std::vector<std::string>> outputs(2);
        for (auto& kv : m_results)
        {
            if (!kv.first.empty())
            {
                outputs[0].emplace_back(kv.first);
                std::string label = kv.second;
                label.erase(
                    std::remove_if(label.begin(), label.end(), [](char ch) { return ch == '\0'; }),
                    label.end());
                outputs[1].emplace_back(label);
            }
        }
        result->setOutputs(std::move(outputs));
    }
    m_taskState->onTaskFinished(result, true);
}