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
 * @file LabeledPSISender.cpp
 * @author: shawnhe
 * @date 2022-11-6
 */


#include <apsi/util/utils.h>
#include <tbb/parallel_for.h>

#include "LabeledPSIParams.h"
#include "LabeledPSISender.h"
#include "QueryPackage.h"
#include "ppc-tools/src/common/TransTools.h"
#include "wedpr-protocol/tars/TarsSerialize.h"

using namespace ppc::psi;
using namespace ppc::protocol;
using namespace ppc::front;
using namespace ppc::tools;
using namespace ppc::crypto;

LabeledPSISender::LabeledPSISender(LabeledPSIConfig::Ptr _config,
    std::function<void(const std::string&, bcos::Error::Ptr)> _taskEndTrigger)
  : m_config(std::move(_config)), m_taskEndTrigger(std::move(_taskEndTrigger))
{
    m_messageFactory = std::make_shared<PPCMessageFactory>();
}

void LabeledPSISender::handlePsiParamsRequest(const PPCMessageFace::Ptr& _message)
{
    auto taskID = _message->taskID();
    LABELED_PSI_LOG(INFO) << LOG_BADGE("handlePsiParamsRequest") << LOG_KV("taskID", taskID);

    try
    {
        auto tarsParams = fromPSIParams(m_senderDB->getParams(), m_senderDB->getBinBundleCount());
        auto paramsMessage = m_messageFactory->buildPPCMessage(uint8_t(TaskType::PSI),
            uint8_t(TaskAlgorithmType::LABELED_PSI_2PC), taskID, std::make_shared<bcos::bytes>());
        paramsMessage->setMessageType(uint8_t(LabeledPSIMessageType::PARAMS_RESPONSE));
        ppctars::serialize::encode(tarsParams, *paramsMessage->data());

        // send psi params response
        m_config->front()->asyncSendMessage(
            _message->sender(), paramsMessage, m_config->networkTimeout(),
            [self = weak_from_this(), taskID](bcos::Error::Ptr _error) {
                auto sender = self.lock();
                if (!sender)
                {
                    return;
                }
                if (_error && _error->errorCode())
                {
                    sender->onSenderTaskDone(taskID, std::move(_error));
                }
            },
            nullptr);
    }
    catch (const std::exception& e)
    {
        onSenderException(taskID, "handlePsiParamsRequest", e);
    }
}

void LabeledPSISender::handleBlindedItems(const front::PPCMessageFace::Ptr& _message)
{
    auto taskID = _message->taskID();
    LABELED_PSI_LOG(INFO) << LOG_BADGE("handleBlindedItems") << LOG_KV("taskID", taskID);

    try
    {
        ppctars::OprfData blindedData;
        ppctars::serialize::decode(*_message->data(), blindedData);

        // evaluate items
        ppctars::OprfData evaluatedData;
        evaluatedData.data = m_config->oprfServer()->evaluate(blindedData.data);

        auto evaluatedMessage = m_messageFactory->buildPPCMessage(uint8_t(TaskType::PSI),
            uint8_t(TaskAlgorithmType::LABELED_PSI_2PC), taskID, std::make_shared<bcos::bytes>());
        evaluatedMessage->setMessageType(uint8_t(LabeledPSIMessageType::OPRF_EVALUATED_ITEMS));
        ppctars::serialize::encode(evaluatedData, *evaluatedMessage->data());

        // send evaluated items
        m_config->front()->asyncSendMessage(
            _message->sender(), evaluatedMessage, m_config->networkTimeout(),
            [self = weak_from_this(), taskID](bcos::Error::Ptr _error) {
                auto sender = self.lock();
                if (!sender)
                {
                    return;
                }
                if (_error && _error->errorCode())
                {
                    sender->onSenderTaskDone(taskID, std::move(_error));
                }
            },
            nullptr);
    }
    catch (const std::exception& e)
    {
        onSenderException(taskID, "handleBlindedItems", e);
    }
}

// receive encrypted powers and send polynomial ciphertext response
void LabeledPSISender::handleQuery(const front::PPCMessageFace::Ptr& _message)
{
    auto taskID = _message->taskID();
    LABELED_PSI_LOG(INFO) << LOG_BADGE("onReceiveQueryRequest") << LOG_KV("taskID", taskID);

    try
    {
        ppctars::QueryRequest queryRequest;
        ppctars::serialize::decode(*(_message->data()), queryRequest);

        // construct queryPackage
        QueryPackage queryPackage(queryRequest, m_senderDB);

        // acquire read lock on SenderDB
        auto senderDBlock = m_senderDB->getReaderLock();

        LABELED_PSI_LOG(INFO) << LOG_DESC("start processing query request on database")
                              << LOG_KV("taskID", taskID)
                              << LOG_KV("itemSize", m_senderDB->getItemCount());

        // Copy over the CryptoContext from SenderDB; set the Evaluator for this local
        // instance. Relinearization keys may not have been included in the query. In
        // that case query.relin_keys() simply holds an empty seal::RelinKeys
        // instance. There is no problem with the below call to CryptoContext::set_evaluator.
        apsi::CryptoContext cryptoContext(m_senderDB->getCryptoContext());
        cryptoContext.set_evaluator(queryPackage.relinKeys());

        uint32_t bundleIdxCount = m_senderDB->getParams().bundle_idx_count();
        uint32_t maxItemsPerBin = m_senderDB->getParams().table_params().max_items_per_bin;

        // For each bundle index i, we need a vector of powers of the query Qᵢ. We
        // need powers all the way up to Qᵢ^max_items_per_bin. We don't store the
        // zeroth power. If Paterson-Stockmeyer is used, then only a subset of the
        // powers will be populated.
        auto allPowers =
            std::make_shared<std::vector<std::vector<seal::Ciphertext>>>(bundleIdxCount);

        // we use a custom SEAL memory that is freed after the query is done
        auto memoryPool = std::make_shared<seal::MemoryPoolHandle>(
            seal::MemoryManager::GetPool(seal::mm_force_new));

        // Initialize powers
        for (auto& powers : *allPowers)
        {
            // The + 1 is because we index by power. The 0th power is a dummy value. I
            // promise this makes things easier to read.
            size_t powersSize = static_cast<size_t>(maxItemsPerBin) + 1;
            powers.reserve(powersSize);
            for (size_t i = 0; i < powersSize; i++)
            {
                powers.emplace_back(*memoryPool);
            }
        }

        // Load inputs provided in the query
        for (auto& powerIt : queryPackage.powers())
        {
            // The exponent of all the query powers we're about to iterate through
            auto exponent = static_cast<size_t>(powerIt.first);

            // Load Qᵢᵉ for all bundle indices i, where e is the exponent specified
            // above
            for (size_t bundleIdx = 0; bundleIdx < bundleIdxCount; ++bundleIdx)
            {
                // Load input^power to allPowers[bundle_idx][exponent]
                LABELED_PSI_LOG(DEBUG)
                    << LOG_DESC("extracting query ciphertext") << LOG_KV("power", exponent)
                    << LOG_KV("bundleIndex", bundleIdx);

                (*allPowers)[bundleIdx][exponent] = powerIt.second[bundleIdx];
            }
        }

        auto receiverID = _message->sender();

        // Compute query powers for the bundle indexes
        for (size_t bundleIdx = 0; bundleIdx < bundleIdxCount; ++bundleIdx)
        {
            computePowers(taskID, receiverID, cryptoContext, allPowers, memoryPool,
                queryPackage.powersDag(), static_cast<uint32_t>(bundleIdx));
        }

        LABELED_PSI_LOG(INFO) << LOG_DESC("start processing bin bundle caches")
                              << LOG_KV("taskID", taskID);
        uint32_t seq = 0;
        auto current = std::make_shared<std::atomic<uint32_t>>(0);
        uint32_t total = m_senderDB->getBinBundleCount();
        for (size_t bundleIdx = 0; bundleIdx < bundleIdxCount; ++bundleIdx)
        {
            auto bundleCaches = m_senderDB->getCacheAt(static_cast<uint32_t>(bundleIdx));
            for (auto& cache : bundleCaches)
            {
                m_config->threadPool()->enqueue([self = weak_from_this(), taskID, receiverID,
                                                    allPowers, memoryPool, cryptoContext, cache,
                                                    bundleIdx, seq, current, total]() {
                    auto sender = self.lock();
                    if (!sender)
                    {
                        return;
                    }

                    try
                    {
                        sender->processBinBundleCache(taskID, receiverID, cryptoContext, allPowers,
                            memoryPool, cache, bundleIdx, seq);

                        auto currentStep = current->fetch_add(1);
                        if (currentStep + 1 == total)
                        {
                            // current task is done for sender
                            sender->onSenderTaskDone(taskID, nullptr);
                        }
                    }
                    catch (const std::exception& e)
                    {
                        sender->onSenderException(taskID,
                            "processBinBundleCache, bundleIdx: " + std::to_string(bundleIdx), e);
                    }
                });
                seq++;
            }
        }
    }
    catch (const std::exception& e)
    {
        onSenderException(taskID, "handleBlindedItems", e);
    }
}


void LabeledPSISender::computePowers(const std::string& _taskID, const std::string& _receiverID,
    const apsi::CryptoContext& _cryptoContext,
    std::shared_ptr<std::vector<std::vector<seal::Ciphertext>>> _allPowers,
    std::shared_ptr<seal::MemoryPoolHandle> _memoryPool, const apsi::PowersDag& _powersDag,
    uint32_t _bundleIdx)
{
    auto bundleCaches = m_senderDB->getCacheAt(_bundleIdx);
    if (bundleCaches.empty())
    {
        return;
    }

    LABELED_PSI_LOG(INFO) << LOG_DESC("start computing powers") << LOG_KV("taskID", _taskID)
                          << LOG_KV("bundleIdx", _bundleIdx);

    auto evaluator = _cryptoContext.evaluator();
    auto relinKeys = _cryptoContext.relin_keys();
    bool relinearize = _cryptoContext.seal_context()->using_keyswitching();

    _powersDag.parallel_apply([&](const apsi::PowersDag::PowersNode& node) {
        if (!node.is_source())
        {
            auto parents = node.parents;
            seal::Ciphertext prod(*_memoryPool);
            if (parents.first == parents.second)
            {
                evaluator->square((*_allPowers)[_bundleIdx][parents.first], prod, *_memoryPool);
            }
            else
            {
                evaluator->multiply((*_allPowers)[_bundleIdx][parents.first],
                    (*_allPowers)[_bundleIdx][parents.second], prod, *_memoryPool);
            }
            if (relinearize)
            {
                evaluator->relinearize_inplace(prod, *relinKeys, *_memoryPool);
            }
            (*_allPowers)[_bundleIdx][node.power] = std::move(prod);
        }
    });

    // Now that all powers of the ciphertext have been computed, we need to
    // transform them to NTT form. This will substantially improve the polynomial
    // evaluation, because the plaintext polynomials are already in NTT
    // transformed form, and the ciphertexts are used repeatedly for each bin
    // bundle at this index. This computation is separate from the graph
    // processing above, because the multiplications must all be done before
    // transforming to NTT form. We omit the first ciphertext in the vector,
    // because it corresponds to the zeroth power of the query and is included
    // only for convenience of the indexing; the ciphertext is actually not set or
    // valid for use.

    // After computing all powers we do modules switch down to parameters that
    // one more level for low powers than for high powers; same choice must be
    // used when encoding/NTT transforming the SenderDB data.
    auto highPowersParmsId =
        apsi::util::get_parms_id_for_chain_idx(*_cryptoContext.seal_context(), 1);
    auto lowPowersParmsId =
        apsi::util::get_parms_id_for_chain_idx(*_cryptoContext.seal_context(), 2);

    uint32_t psLowDegree = m_senderDB->getParams().query_params().ps_low_degree;

    std::vector<std::uint32_t> targetPowers;
    for (auto& power : _powersDag.target_powers())
    {
        targetPowers.emplace_back(power);
    }

    tbb::parallel_for(tbb::blocked_range<size_t>(0U, targetPowers.size()), [&](auto const& range) {
        for (auto i = range.begin(); i < range.end(); i++)
        {
            uint32_t power = targetPowers[i];

            if (!psLowDegree)
            {
                // Only one ciphertext-plaintext multiplication is needed after this
                evaluator->mod_switch_to_inplace(
                    (*_allPowers)[_bundleIdx][power], highPowersParmsId, *_memoryPool);

                // All powers must be in NTT form
                evaluator->transform_to_ntt_inplace((*_allPowers)[_bundleIdx][power]);
            }
            else
            {
                if (power <= psLowDegree)
                {
                    // Low powers must be at a higher level than high powers
                    evaluator->mod_switch_to_inplace(
                        (*_allPowers)[_bundleIdx][power], lowPowersParmsId, *_memoryPool);

                    // Low powers must be in NTT form
                    evaluator->transform_to_ntt_inplace((*_allPowers)[_bundleIdx][power]);
                }
                else
                {
                    // High powers are only modulus switched
                    evaluator->mod_switch_to_inplace(
                        (*_allPowers)[_bundleIdx][power], highPowersParmsId, *_memoryPool);
                }
            }
        }
    });

    LABELED_PSI_LOG(INFO) << LOG_DESC("finish computing powers") << LOG_KV("taskID", _taskID)
                          << LOG_KV("bundleIdx", _bundleIdx);
}

void LabeledPSISender::processBinBundleCache(const std::string& _taskID,
    const std::string& _receiverID, const apsi::CryptoContext& _cryptoContext,
    const std::shared_ptr<std::vector<std::vector<seal::Ciphertext>>>& _allPowers,
    const std::shared_ptr<seal::MemoryPoolHandle>& _memoryPool,
    std::reference_wrapper<const BinBundleCache> _cache, uint32_t _bundleIdx, uint32_t _seq)
{
    LABELED_PSI_LOG(DEBUG) << LOG_DESC("processBinBundleCache") << LOG_KV("taskID", _taskID)
                           << LOG_KV("bundleIdx", _bundleIdx);

    // Package for the result data
    ResultPackage resultPackage;
    resultPackage.bundleIdx = _bundleIdx;
    resultPackage.nonceByteCount = seal::util::safe_cast<uint32_t>(m_senderDB->getNonceByteCount());
    resultPackage.labelByteCount = seal::util::safe_cast<uint32_t>(m_senderDB->getLabelByteCount());

    // Compute the matching result and move to rp
    const BatchedPlaintextPolyn& matchingPolyn = _cache.get().batched_matching_polyn;

    // Determine if we use Paterson-Stockmeyer or not
    uint32_t psLowDegree = m_senderDB->getParams().query_params().ps_low_degree;
    uint32_t degree = seal::util::safe_cast<uint32_t>(matchingPolyn.batched_coeffs.size()) - 1;

    bool usingPs = (psLowDegree > 1) && (psLowDegree < degree);
    if (usingPs)
    {
        resultPackage.psiCipherResult = matchingPolyn.eval_patstock(_cryptoContext,
            (*_allPowers)[_bundleIdx], seal::util::safe_cast<size_t>(psLowDegree), *_memoryPool);
    }
    else
    {
        resultPackage.psiCipherResult = matchingPolyn.eval((*_allPowers)[_bundleIdx], *_memoryPool);
    }

    for (const auto& interpPolyn : _cache.get().batched_interp_polyns)
    {
        // Compute the label result and move to rp
        degree = seal::util::safe_cast<uint32_t>(interpPolyn.batched_coeffs.size()) - 1;
        usingPs = (psLowDegree > 1) && (psLowDegree < degree);
        if (usingPs)
        {
            resultPackage.labelCipherResults.emplace_back(interpPolyn.eval_patstock(
                _cryptoContext, (*_allPowers)[_bundleIdx], psLowDegree, *_memoryPool));
        }
        else
        {
            resultPackage.labelCipherResults.emplace_back(
                interpPolyn.eval((*_allPowers)[_bundleIdx], *_memoryPool));
        }
    }

    // send result package to receiver
    sendResultPackage(_taskID, _receiverID, resultPackage, _seq);
}

void LabeledPSISender::sendResultPackage(const std::string& _taskID, const std::string& _receiverID,
    const ResultPackage& _resultPackage, uint32_t _seq)
{
    LABELED_PSI_LOG(DEBUG) << LOG_DESC("send result package") << LOG_KV("taskID", _taskID)
                           << LOG_KV("seq", _seq);

    ppctars::QueryResponse queryResponse;
    queryResponse.labelByteCount = _resultPackage.labelByteCount;
    queryResponse.nonceByteCount = _resultPackage.nonceByteCount;
    queryResponse.bundleIdx = _resultPackage.bundleIdx;

    std::vector<uint8_t> psiCipher(_resultPackage.psiCipherResult.save_size(m_comprMode));
    _resultPackage.psiCipherResult.save(psiCipher, m_comprMode);
    queryResponse.ciphertext = std::move(psiCipher);

    for (auto& result : _resultPackage.labelCipherResults)
    {
        std::vector<uint8_t> labelCipher(result.save_size(m_comprMode));
        result.save(labelCipher, m_comprMode);
        queryResponse.labelResults.emplace_back(std::move(labelCipher));
    }

    auto resultMessage = m_messageFactory->buildPPCMessage(uint8_t(TaskType::PSI),
        uint8_t(TaskAlgorithmType::LABELED_PSI_2PC), _taskID, std::make_shared<bcos::bytes>());
    resultMessage->setMessageType(uint8_t(LabeledPSIMessageType::RESPONSE));
    resultMessage->setSeq(_seq);
    ppctars::serialize::encode(queryResponse, *resultMessage->data());

    auto error = m_config->sendMessage(_receiverID, resultMessage);
    if (error && error->errorCode())
    {
        onSenderTaskDone(_taskID, std::move(error));
    }
}

void LabeledPSISender::onSenderException(
    const std::string& _taskID, const std::string& _module, const std::exception& _e)
{
    LABELED_PSI_LOG(ERROR) << LOG_BADGE("onSenderException") << LOG_KV("taskID", _taskID)
                           << LOG_KV("module", _module)
                           << LOG_KV("error", boost::diagnostic_information(_e));
    auto error = std::make_shared<bcos::Error>((int)LabeledPSIRetCode::ON_EXCEPTION,
        "exception caught while do " + _module + ", message: " + boost::diagnostic_information(_e));
    m_taskEndTrigger(_taskID, error);
}

void LabeledPSISender::onSenderTaskDone(const std::string& _taskID, bcos::Error::Ptr _error)
{
    m_taskEndTrigger(_taskID, std::move(_error));
}
