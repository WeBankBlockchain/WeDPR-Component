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
 * @file OpenSSLAES.h
 * @author: shawnhe
 * @date 2022-11-29
 */

#pragma once

#include "../Common.h"
#include "OpenSSLCipher.h"

namespace ppc::crypto
{
class OpenSSLAES : public OpenSSLCipher
{
public:
    using Ptr = std::shared_ptr<OpenSSLAES>;

    enum class AESType : int
    {
        AES128 = 16,
        AES192 = 24,
        AES256 = 32,
    };

    OpenSSLAES() = delete;

    ~OpenSSLAES() override = default;

    OpenSSLAES(
        AESType _aesType, protocol::DataPaddingType _padding = protocol::DataPaddingType::PKCS7)
      : OpenSSLCipher(_padding), m_aesType(_aesType)
    {}

    protocol::SymCryptoImplName type() const override { return protocol::SymCryptoImplName::AES; }

    AESType aesType() { return m_aesType; }

    EvpCipherPtr createCipherMeth(OperationMode _mode) const override;

private:
    AESType m_aesType;
};

}  // namespace ppc::crypto
