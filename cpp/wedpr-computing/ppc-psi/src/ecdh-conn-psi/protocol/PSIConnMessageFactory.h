/*
 *  Copyright (C) 2023 WeDPR.
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
 * @file PSIConnMessageFactory.h
 * @author: zachma
 * @date 2023-8-23
 */

#pragma once
#include "../Common.h"
#include "PSIConnMessage.h"
#include "ppc-framework/Common.h"

namespace ppc::psi
{
#define ECDH_CONN_PB_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("ECDH-CONN-PSI-PB")
class PSIConnMessageFactory
{
public:
    using Ptr = std::shared_ptr<PSIConnMessageFactory>;
    PSIConnMessageFactory() = default;
    virtual ~PSIConnMessageFactory() = default;

    bcos::bytes createHandshakeRequest(HandShakeRequestVo::Ptr handShakeRequestVo);
    HandShakeRequestVo::Ptr parseHandshakeRequest(const bcos::bytes& _value);

    bcos::bytes createHandshakeResponse(HandShakeResponseVo::Ptr _handShakeResponseVo);
    HandShakeResponseVo::Ptr parseHandshakeResponse(const bcos::bytes& _value);

    bcos::bytes createCipherExchange(CipherBatchVo::Ptr _cipherBatchVo);
    CipherBatchVo::Ptr parseCipherExchange(const bcos::bytes& _value);

    bcos::bytes createPSIConnMessageRequest(const bcos::bytes& _value, const std::string& _key);

protected:
    std::string encodeVectorBytesToString(const std::vector<bcos::bytes>& _cipherBytes)
    {
        std::string _temp = "";
        for (auto const& _cipher_byte : _cipherBytes)
        {
            _temp += std::string(_cipher_byte.begin(), _cipher_byte.end());
        }
        return _temp;
    }

    std::vector<bcos::bytes> decodeStringToVectorBytes(const std::string& _cipherStr, int count)
    {
        std::vector<bcos::bytes> result;
        if (count == 0 || _cipherStr == "")
        {
            return result;
        }
        auto _size = _cipherStr.length();
        auto batch = _size / count;
        int start = 0;
        for (int i = 0; i < count; i++)
        {
            std::string temp = _cipherStr.substr(start, batch);
            bcos::bytes temp_bytes(temp.begin(), temp.end());
            result.push_back(temp_bytes);
            start += batch;
        }
        return result;
    }
};
}  // namespace ppc::psi