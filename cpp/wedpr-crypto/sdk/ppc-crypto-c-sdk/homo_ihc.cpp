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
 * @file homo_ihc.cpp
 * @author: asherli
 * @date 2023-11-28
 */
#include "homo_ihc.h"
#include "ppc-homo/ihc/IhcImpl.h"
#include "utils/error.h"

using namespace ppc;
using namespace ppc::homo;
using namespace ppc::crypto;
using namespace bcos;

Ihc::Ptr g_ihc_128 = std::make_shared<IhcImpl>((int)Ihc::IhcMode::IHC_128, 16);
Ihc::Ptr g_ihc_256 = std::make_shared<IhcImpl>((int)Ihc::IhcMode::IHC_256, 16);

Ihc::Ptr obtainIhcInstance(int mode)
{
    switch (mode)
    {
    case (int)Ihc::IhcMode::IHC_128:
        return g_ihc_128;
    case (int)Ihc::IhcMode::IHC_256:
        return g_ihc_256;
    default:
    {
        std::string errorMsg = "Unsupported ihc mode!";
        set_last_error_msg(-1, errorMsg.c_str());
    }
    }
    return nullptr;
}

unsigned int ihc_key_bytes(int mode)
{
    clear_last_error();
    auto pIhc = obtainIhcInstance(mode);
    if (!pIhc)
    {
        return 0;
    }
    return pIhc->keyBytes();
}

uint64_t ihc_cipher_bytes(int mode)
{
    clear_last_error();
    auto pIhc = obtainIhcInstance(mode);
    if (!pIhc)
    {
        return 0;
    }
    return pIhc->cipherBytes();
}

void ihc_generate_key(OutputBuffer* key, int mode)
{
    clear_last_error();
    try
    {
        Ihc::Ptr pIhc = obtainIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        pIhc->generateKey(key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void ihc_encrypt(
    OutputBuffer* cipherBytes, int _mode, InputBuffer const* _key, BIGNUM const* _plain)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainIhcInstance(_mode);
        if (!pIhc)
        {
            return;
        }
        pIhc->encrypt(cipherBytes, bcos::bytesConstRef((bcos::byte*)_key->data, _key->len), _plain);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void ihc_decrypt(BIGNUM* _result, int _mode, InputBuffer const* _key, InputBuffer const* cipher)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainIhcInstance(_mode);
        if (!pIhc)
        {
            return;
        }
        auto decryptedV = pIhc->decrypt(bcos::bytesConstRef((bcos::byte*)_key->data, _key->len),
            bcos::bytesConstRef((bcos::byte*)cipher->data, cipher->len));
        BN_print_fp(stdout, decryptedV.bn().get());
        decryptedV.swap(_result);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

void ihc_add(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainIhcInstance(mode);
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

void ihc_sub(
    OutputBuffer* cipherBytes, int mode, InputBuffer const* cipher1, InputBuffer const* cipher2)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainIhcInstance(mode);
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

void ihc_scalaMul(OutputBuffer* cipherBytes, int mode, BIGNUM const* v, InputBuffer const* cipher)
{
    clear_last_error();
    try
    {
        auto pIhc = obtainIhcInstance(mode);
        if (!pIhc)
        {
            return;
        }
        pIhc->scalaMul(
            cipherBytes, v, bcos::bytesConstRef((bcos::byte const*)cipher->data, cipher->len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}