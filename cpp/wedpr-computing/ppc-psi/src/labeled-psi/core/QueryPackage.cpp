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
 * @file QueryPackage.cpp
 * @author: shawnhe
 * @date 2022-11-14
 *
 */

#include <apsi/powers.h>
#include <apsi/seal_object.h>
#include <apsi/util/label_encryptor.h>

#include "QueryPackage.h"
#include "ppc-psi/src/labeled-psi/Common.h"

using namespace ppc::psi;

QueryPackage::QueryPackage(ppctars::QueryRequest _queryQuest, SenderDB::Ptr _senderDB)
{
    auto sealContext = _senderDB->getSealContext();

    // extract and validate relinearization keys
    apsi::SEALObject<seal::RelinKeys> relinKeys;
    if (sealContext->using_keyswitching())
    {
        gsl::span<const uint8_t> relinKeysSpan(
            reinterpret_cast<const uint8_t*>(_queryQuest.relinKeys.data()),
            _queryQuest.relinKeys.size());
        relinKeys.load(sealContext, relinKeysSpan);
        m_relinKeys = relinKeys.extract(sealContext);
        if (!is_valid_for(m_relinKeys, *sealContext))
        {
            BOOST_THROW_EXCEPTION(ExtracteRelinKeysException() << bcos::errinfo_comment(
                                      "Extracted RelinKeys are invalid for SEALContext"));
        }
    }

    // extract and validate query ciphertexts
    for (auto& encryptedPowers : _queryQuest.encryptedPowers)
    {
        std::vector<seal::Ciphertext> ciphertexts;
        ciphertexts.reserve(encryptedPowers.ciphertexts.size());
        for (auto& ct : encryptedPowers.ciphertexts)
        {
            gsl::span<const uint8_t> ctSpan(reinterpret_cast<const uint8_t*>(ct.data()), ct.size());
            apsi::SEALObject<seal::Ciphertext> temp;
            temp.load(sealContext, ctSpan);
            ciphertexts.emplace_back(temp.extract(sealContext));
            if (!is_valid_for(ciphertexts.back(), *sealContext))
            {
                BOOST_THROW_EXCEPTION(ExtracteCiphertextException() << bcos::errinfo_comment(
                                          "Extracted ciphertext is invalid for SEALContext"));
            }
        }
        m_powers.emplace(encryptedPowers.power, std::move(ciphertexts));
    }

    uint32_t psLowDegree = _senderDB->getParams().query_params().ps_low_degree;
    uint32_t maxItemsPerBin = _senderDB->getParams().table_params().max_items_per_bin;

    std::set<uint32_t> targetPowers = create_powers_set(psLowDegree, maxItemsPerBin);
    const std::set<uint32_t>& queryPowers = _senderDB->getParams().query_params().query_powers;

    // create the PowersDag
    m_powersDag.configure(queryPowers, targetPowers);

    // check that the PowersDag is valid
    if (!m_powersDag.is_configured())
    {
        BOOST_THROW_EXCEPTION(PowerDagException() << bcos::errinfo_comment(
                                  "Failed to configure PowersDag, queryPowers: " +
                                  apsi::util::to_string(queryPowers) +
                                  ", targetPowers: " + apsi::util::to_string(targetPowers)));
    }
    LABELED_PSI_LOG(INFO) << LOG_DESC("finished configuring PowersDag")
                          << LOG_KV("depth", m_powersDag.depth());

    // check that the query data size matches the PSIParams

    if (m_powers.size() != queryPowers.size())
    {
        BOOST_THROW_EXCEPTION(
            QueryPowersException() << bcos::errinfo_comment(
                "Extracted query data is incompatible with PSI parameters, powderDag contains " +
                std::to_string(m_powers.size()) +
                " ciphertext powers which does not match with the size of queryPowers (" +
                std::to_string(queryPowers.size()) + ")"));
    }
    uint32_t bundleIdxCount = _senderDB->getParams().bundle_idx_count();
    for (auto& powers : m_powers)
    {
        // check that powers in the query data match source nodes in the PowersDag
        if (powers.second.size() != bundleIdxCount)
        {
            BOOST_THROW_EXCEPTION(
                QueryPowersException() << bcos::errinfo_comment(
                    "Extracted query data is incompatible with PSI parameters, queryPower " +
                    std::to_string(powers.first) + " contains " +
                    std::to_string(powers.second.size()) +
                    " ciphertexts which does not match with bundle_idx_count (" +
                    std::to_string(bundleIdxCount) + ")"));
        }

        auto where = find_if(queryPowers.cbegin(), queryPowers.cend(),
            [&powers](auto n) { return n == powers.first; });
        if (where == queryPowers.cend())
        {
            BOOST_THROW_EXCEPTION(
                QueryPowersException() << bcos::errinfo_comment(
                    "Extracted query data is incompatible with PowersDag, queryPower " +
                    std::to_string(powers.first) +
                    " does not match with a source node in PowersDag"));
        }
    }
}