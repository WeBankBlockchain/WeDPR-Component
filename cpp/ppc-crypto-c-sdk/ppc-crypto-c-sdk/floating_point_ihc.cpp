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
 * @file floating_point_ihc.cpp
 * @author: asherli
 * @date 2023-11-28
 */
#include "floating_point_ihc.h"
#include "ppc-framework/libwrapper/FloatingPointNumber.h"
#include "ppc-homo/ihc/FloatingPointIhc.h"
#include "utils/error.h"

using namespace ppc;
using namespace ppc::homo;
using namespace ppc::crypto;
using namespace bcos;

FloatingPointIhc::Ptr g_fp_ihc_128 =
    std::make_shared<FloatingPointIhc>(std::make_shared<IhcImpl>((int)Ihc::IhcMode::IHC_128, 16));
FloatingPointIhc::Ptr g_fp_ihc_256 =
    std::make_shared<FloatingPointIhc>(std::make_shared<IhcImpl>((int)Ihc::IhcMode::IHC_256, 16));

FloatingPointIhc::Ptr obtainFpIhcInstance(int mode)
{
    switch (mode)
    {
    case (int)Ihc::IhcMode::IHC_128:
        return g_fp_ihc_128;
    case (int)Ihc::IhcMode::IHC_256:
        return g_fp_ihc_256;
    default:
    {
        std::string errorMsg = "Unsupported ihc mode!";
        set_last_error_msg(-1, errorMsg.c_str());
    }
    }
    return nullptr;
}

uint64_t ihc_floating_cipher_bytes(int mode)
{
    clear_last_error();
    auto pIhc = obtainFpIhcInstance(mode);
    if (!pIhc)
    {
        return 0;
    }
    return pIhc->cipherBytes();
}

void ihc_floating_encrypt(OutputBuffer* cipherBytes, int mode, InputBuffer const* _key,
    BIGNUM const* value, int16_t _exponent)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainFpIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        BigNum v;
        BN_copy(v.bn().get(), value);
        FloatingPointNumber ffpNumber(std::move(v), _exponent);
        pIhc->encrypt(
            cipherBytes, bcos::bytesConstRef((bcos::byte const*)_key->data, _key->len), ffpNumber);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void ihc_floating_decrypt(
    BIGNUM* value, int16_t* exponent, int mode, InputBuffer const* key, InputBuffer const* cipher)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainFpIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        auto result = pIhc->decrypt(bcos::bytesConstRef((bcos::byte const*)key->data, key->len),
            bcos::bytesConstRef((bcos::byte const*)cipher->data, cipher->len));
        result.value.swap(value);
        *exponent = result.exponent;
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
void ihc_floating_add(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainFpIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        pIhc->add(cipherBytes, bcos::bytesConstRef((bcos::byte const*)cipher1->data, cipher1->len),
            bcos::bytesConstRef((bcos::byte const*)cipher2->data, cipher2->len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}
void ihc_floating_sub(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainFpIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        pIhc->sub(cipherBytes, bcos::bytesConstRef((bcos::byte const*)cipher1->data, cipher1->len),
            bcos::bytesConstRef((bcos::byte const*)cipher2->data, cipher2->len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void ihc_floating_scalaMul(OutputBuffer* cipherBytes, int mode, BIGNUM const* v, int16_t exponent,
    InputBuffer const* cipher)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainFpIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        BigNum value;
        BN_copy(value.bn().get(), v);
        FloatingPointNumber vFpNumber(std::move(value), exponent);
        pIhc->scalaMul(cipherBytes, vFpNumber,
            bcos::bytesConstRef((bcos::byte const*)cipher->data, cipher->len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}