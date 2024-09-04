/**
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
 * @file FloatingPointCipher.h
 * @author: yujiechen
 * @date 2023-08-17
 */
#pragma once
#include "ppc-framework/libwrapper/Buffer.h"
#include "ppc-tools/src/codec/CodecUtility.h"
#include <bcos-utilities/Common.h>
#include <memory>
namespace ppc::homo
{
DERIVE_PPC_EXCEPTION(FloatingPointCipherException);
class FloatingPointCipher
{
public:
    using Ptr = std::shared_ptr<FloatingPointCipher>;
    FloatingPointCipher(bcos::bytes&& _cipher, int16_t _exponent)
      : m_cipher(std::move(_cipher)), m_exponent(_exponent)
    {}
    FloatingPointCipher(bcos::bytesConstRef const& _data) { decode(_data); }

    virtual ~FloatingPointCipher() = default;

    virtual void decode(bcos::bytesConstRef const& _data)
    {
        auto bufferLen = _data.size();
        // exponent
        auto offset = decodeInteger(m_exponent, _data.data(), bufferLen, 0);
        // cipher
        decodeBuffer(m_cipher, _data.data(), bufferLen, offset);
    }

    virtual void encode(OutputBuffer* _output) const
    {
        auto pEnd = _output->data + _output->len;
        // exponent
        auto p = encodeInteger(_output->data, pEnd, m_exponent);
        // cipher
        p = encodeBuffer(p, pEnd, bcos::ref(m_cipher));
        _output->len = (p - _output->data);
    }

    int maxEncodedSize() const { return m_cipher.size() + sizeof(uint16_t) + sizeof(int16_t); }

    bcos::bytes const& cipher() const { return m_cipher; }
    int16_t exponent() const { return m_exponent; }

    void setCipher(bcos::bytes&& _cipher) { m_cipher = std::move(_cipher); }
    void setExponent(int16_t _exponent) { m_exponent = _exponent; }

private:
    // the cipher
    bcos::bytes m_cipher;
    // the exponent
    int16_t m_exponent;
};
}  // namespace ppc::homo