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
 * @file ResultPackage.cpp
 * @author: shawnhe
 * @date 2022-11-6
 */

#include "ResultPackage.h"
#include "ppc-psi/src/labeled-psi/Common.h"

using namespace ppc::psi;

PlainResultPackage ResultPackage::extract(const apsi::CryptoContext& _cryptoContext)
{
    if (!_cryptoContext.decryptor())
    {
        BOOST_THROW_EXCEPTION(ResultPackageException() << bcos::errinfo_comment(
                                  "Decryptor is not configured in CryptoContext"));
    }

    PlainResultPackage plainRp;
    plainRp.bundleIdx = bundleIdx;
    plainRp.labelByteCount = labelByteCount;
    plainRp.nonceByteCount = nonceByteCount;

    // psi
    seal::Plaintext psiPlain;
    seal::Ciphertext psiCipher = psiCipherResult.extract(_cryptoContext.seal_context());
    _cryptoContext.decryptor()->decrypt(psiCipher, psiPlain);
    _cryptoContext.encoder()->decode(psiPlain, plainRp.psiResult);

    // labels
    for (auto& ct : labelCipherResults)
    {
        seal::Plaintext labelPlain;
        seal::Ciphertext labelCipher = ct.extract(_cryptoContext.seal_context());
        _cryptoContext.decryptor()->decrypt(labelCipher, labelPlain);

        std::vector<uint64_t> labelResultData;
        _cryptoContext.encoder()->decode(labelPlain, labelResultData);
        plainRp.labelResults.emplace_back(std::move(labelResultData));
    }

    // clear the label data
    std::vector<apsi::SEALObject<seal::Ciphertext>>().swap(labelCipherResults);

    return plainRp;
}
