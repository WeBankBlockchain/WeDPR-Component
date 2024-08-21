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
 * @file CryptoBox.h
 * @author: yujiechen
 * @date 2022-11-7
 */
#pragma once
#include "EccCrypto.h"
#include "Hash.h"
namespace ppc::crypto
{
class CryptoBox
{
public:
    using Ptr = std::shared_ptr<CryptoBox>;
    CryptoBox() = delete;
    CryptoBox(Hash::Ptr const& _hashImpl, EccCrypto::Ptr const& _eccCrypto)
      : m_hashImpl(_hashImpl), m_eccCrypto(_eccCrypto)
    {}

    Hash::Ptr const& hashImpl() const { return m_hashImpl; }
    EccCrypto::Ptr const& eccCrypto() const { return m_eccCrypto; }

private:
    Hash::Ptr m_hashImpl;
    EccCrypto::Ptr m_eccCrypto;
};
}  // namespace ppc::crypto