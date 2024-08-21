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
 * @file FloatingPointIhc.h
 * @author: asherli
 * @date 2023-12-29
 */
#pragma once
#include "../codec/Common.h"
#include "IhcImpl.h"
#include "openssl/bn.h"
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include <memory>
namespace ppc::homo
{
class FloatingPointIhc : public std::enable_shared_from_this<FloatingPointIhc>
{
public:
    using Ptr = std::shared_ptr<FloatingPointIhc>;
    FloatingPointIhc(Ihc::Ptr _ihc) : m_ihc(std::move(_ihc)) {}
    virtual ~FloatingPointIhc() = default;

    virtual void encrypt(OutputBuffer* _result, bcos::bytesConstRef const& sk,
        ppc::FloatingPointNumber const& _value) const;

    virtual bcos::bytes encrypt(
        bcos::bytesConstRef const& sk, ppc::FloatingPointNumber const& _value) const
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer resultBuffer{result.data(), result.size()};
        encrypt(&resultBuffer, sk, _value);
        result.resize(resultBuffer.len);
        return result;
    }

    virtual ppc::FloatingPointNumber decrypt(
        bcos::bytesConstRef const& _sk, bcos::bytesConstRef const& _cipher) const;

    virtual void add(OutputBuffer* _result, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2) const;

    virtual bcos::bytes add(bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2) const
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer resultBuffer{result.data(), result.size()};
        add(&resultBuffer, _c1, _c2);
        result.resize(resultBuffer.len);
        return result;
    }

    virtual void sub(OutputBuffer* _result, bcos::bytesConstRef const& _c1,
        bcos::bytesConstRef const& _c2) const;
    virtual bcos::bytes sub(bcos::bytesConstRef const& _c1, bcos::bytesConstRef const& _c2)
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer resultBuffer{result.data(), result.size()};
        sub(&resultBuffer, _c1, _c2);
        result.resize(resultBuffer.len);
        return result;
    }

    virtual void scalaMul(OutputBuffer* _result, ppc::FloatingPointNumber const& _v,
        bcos::bytesConstRef const& _c) const;

    virtual bcos::bytes scalaMul(ppc::FloatingPointNumber const& _v, bcos::bytesConstRef const& _c)
    {
        bcos::bytes result(cipherBytes());
        OutputBuffer resultBuffer{result.data(), result.size()};
        scalaMul(&resultBuffer, _v, _c);
        result.resize(resultBuffer.len);
        return result;
    }

    virtual uint64_t cipherBytes() const
    {
        return m_ihc->cipherBytes() + sizeof(uint16_t) + sizeof(int16_t);
    }

private:
    inline void align(FloatingPointCipher& _c1, FloatingPointCipher& _c2) const
    {
        auto self = weak_from_this();
        precisionAlign(
            _c1, _c2, [self](BIGNUM const* v, bcos::bytesConstRef const& _cipher) -> bcos::bytes {
                auto impl = self.lock();
                if (!impl)
                {
                    return bcos::bytes();
                }
                return impl->m_ihc->scalaMul(v, _cipher);
            });
    }

private:
    Ihc::Ptr m_ihc;
};
}  // namespace ppc::homo