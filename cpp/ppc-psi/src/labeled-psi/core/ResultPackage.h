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
 * @file ResultPackage.h
 * @author: shawnhe
 * @date 2022-11-6
 */

#pragma once
#include <memory>
#include <vector>

#include <apsi/crypto_context.h>
#include <apsi/seal_object.h>
#include <seal/seal.h>
#include <gsl/span>

namespace ppc::psi
{
/**
Stores a decrypted and decoded PSI response and optionally a labeled PSI response.
*/
struct PlainResultPackage
{
    std::uint32_t bundleIdx;

    std::vector<std::uint64_t> psiResult;

    std::uint32_t labelByteCount;

    std::uint32_t nonceByteCount;

    std::vector<std::vector<std::uint64_t>> labelResults;
};

/**
Stores a PSI response and optionally labeled PSI response ciphertexts.
*/
struct ResultPackage
{
public:
    PlainResultPackage extract(const apsi::CryptoContext& crypto_context);

    std::uint32_t bundleIdx;

    apsi::SEALObject<seal::Ciphertext> psiCipherResult;

    std::uint32_t labelByteCount;

    std::uint32_t nonceByteCount;

    std::vector<apsi::SEALObject<seal::Ciphertext>> labelCipherResults;
};


}  // namespace ppc::psi