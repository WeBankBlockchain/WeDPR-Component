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
 * @file OpenSSLSM4.h
 * @author: shawnhe
 * @date 2022-11-29
 */


#pragma once

#include "../Common.h"
#include "OpenSSLCipher.h"

namespace ppc::crypto
{
class OpenSSLSM4 : public OpenSSLCipher
{
public:
    using Ptr = std::shared_ptr<OpenSSLSM4>;
    OpenSSLSM4(protocol::DataPaddingType _padding = protocol::DataPaddingType::PKCS7)
      : OpenSSLCipher(_padding)
    {}

    ~OpenSSLSM4() override = default;
    protocol::SymCryptoImplName type() const override { return protocol::SymCryptoImplName::SM4; }

private:
    EvpCipherPtr createCipherMeth(OperationMode _mode) const override;
};

}  // namespace ppc::crypto
