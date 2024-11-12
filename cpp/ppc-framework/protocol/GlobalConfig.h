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
 * @file GlobalConfig.h
 * @author: yujiechen
 * @date 2023-1-3
 */
#pragma once
#include "../Common.h"
#include "Protocol.h"
namespace ppc::protocol
{
class GlobalConfig
{
public:
    static GlobalConfig& instance()
    {
        static GlobalConfig ins;
        return ins;
    }
    GlobalConfig()
    {
        // set the supported-curves for ecdh-psi
        auto key = calculateKey((uint8_t)TaskType::PSI, (uint8_t)TaskAlgorithmType::ECDH_PSI_2PC);
#ifdef ENABLE_CRYPTO_MB
#ifdef ENABLE_CPU_FEATURES
        if (ppc::CPU_FEATURES.avx512ifma)
        {
            c_supportedCurves[key] = {(int)ECCCurve::IPP_X25519};
        }
#endif
#endif
        c_supportedCurves[key].emplace_back((int)ECCCurve::P256);
        c_supportedCurves[key].emplace_back((int)ECCCurve::ED25519);
        c_supportedCurves[key].emplace_back((int)ECCCurve::SM2);
        c_supportedCurves[key].emplace_back((int)ECCCurve::SECP256K1);

        // set the supported-hash algorithm for ecdh-psi
        c_supportedHashList[key] = {(int)HashImplName::BLAKE2b, (int)HashImplName::SHA256,
            (int)HashImplName::SM3, (int)HashImplName::SHA512};
    }

    std::vector<int> supportedCurves(uint8_t _taskType, uint8_t _algorithmType)
    {
        auto key = calculateKey(_taskType, _algorithmType);
        auto it = c_supportedCurves.find(key);
        if (it != c_supportedCurves.end())
        {
            return it->second;
        }
        return std::vector<int>();
    }

    std::vector<int> supportedHashList(uint8_t _taskType, uint8_t _algorithmType)
    {
        auto key = calculateKey(_taskType, _algorithmType);
        auto it = c_supportedHashList.find(key);
        if (it != c_supportedHashList.end())
        {
            return it->second;
        }
        return std::vector<int>();
    }

    void setSMCrypto(bool _smCrypto) { m_smCrypto = _smCrypto; }
    bool smCrypto() const { return m_smCrypto; }

private:
    uint16_t calculateKey(uint8_t _taskType, uint8_t _algorithmType)
    {
        return (uint16_t)_taskType << 8 | (uint16_t)(_algorithmType);
    }

private:
    std::map<uint16_t, std::vector<int>> c_supportedCurves;
    std::map<uint16_t, std::vector<int>> c_supportedHashList;
    bool m_smCrypto = false;
};
}  // namespace ppc::protocol
#define g_PPCConfig ppc::protocol::GlobalConfig::instance()