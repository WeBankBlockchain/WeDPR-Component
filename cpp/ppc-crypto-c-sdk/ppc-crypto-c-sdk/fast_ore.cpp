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
 * @file fast_ore.cpp
 * @author: yujiechen
 * @date 2023-09-12
 */
#include "fast_ore.h"
#include "ppc-crypto-core/src/ore/FastOre.h"
#include "utils/error.h"
#include "utils/utilities.h"
#include <bcos-utilities/Log.h>
#include <thread>

using namespace ppc;
using namespace ppc::crypto;

thread_local FastOre::Ptr g_ore_impl = std::make_shared<FastOre>();

int fast_ore_key_bytes()
{
    clear_last_error();
    try
    {
        return g_ore_impl->keyBytes();
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
    return -1;
}

uint64_t fast_ore_get_cipher_size(uint64_t plainSize, bool hex)
{
    clear_last_error();
    return g_ore_impl->estimatedCipherSize(plainSize, hex);
}

uint64_t fast_ore_get_float_cipher_size(uint64_t _plainSize, bool hex)
{
    clear_last_error();
    return g_ore_impl->estimatedFloatCipherSize(_plainSize, hex);
}

uint64_t fast_ore_get_plain_size(uint64_t cipherSize, bool hex)
{
    clear_last_error();
    return g_ore_impl->estimatedPlainSize(cipherSize, hex);
}
/**
 * @brief generate key for the fastORE
 *
 * @param key the generated key
 */
void fast_ore_generate_key(OutputBuffer* key)
{
    clear_last_error();
    try
    {
        g_ore_impl->generateKey(key);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief encrypt the plain data into cipher data using given key
 *
 * @param cipher the encrypted data
 * @param key the key used to encrypt
 * @param plain the plain-data
 */
void string_fast_ore_encrypt(
    OutputBuffer* cipher, InputBuffer const* key, InputBuffer const* plain, bool hexEncode)
{
    clear_last_error();
    try
    {
        auto refPlain = bcos::bytesConstRef((bcos::byte*)plain->data, plain->len);
        if (!hexEncode)
        {
            g_ore_impl->encrypt4String(
                cipher, bcos::bytesConstRef((bcos::byte*)key->data, key->len), refPlain);
            return;
        }
        auto hexedBufferSize = g_ore_impl->estimatedCipherSize(refPlain.size(), true);
        if (cipher->len < hexedBufferSize)
        {
            auto errorMsg = "string_fast_ore_encrypt error for unenough cipher buffer, at least: " +
                            std::to_string(hexedBufferSize);
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }
        bcos::bytes bytesResult(g_ore_impl->estimatedCipherSize(refPlain.size(), false));
        OutputBuffer bytesResultBuffer{bytesResult.data(), bytesResult.size()};
        g_ore_impl->encrypt4String(
            &bytesResultBuffer, bcos::bytesConstRef((bcos::byte*)key->data, key->len), refPlain);
        InputBuffer buffer{bytesResultBuffer.data, bytesResultBuffer.len};
        to_hex(cipher, &buffer);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief decrypt the cipher data into plain data using given key
 *
 * @param plain the decrypted plain data
 * @param key the key used to decrypt
 * @param cipher the cipher
 */
void string_fast_ore_decrypt(
    OutputBuffer* plain, InputBuffer const* key, InputBuffer const* cipher, bool hexEncode)
{
    clear_last_error();
    try
    {
        if (!hexEncode)
        {
            return g_ore_impl->decrypt4String(plain,
                bcos::bytesConstRef((bcos::byte*)key->data, key->len),
                bcos::bytesConstRef((bcos::byte*)cipher->data, cipher->len));
        }
        // convert the cipher from hex to binary
        bcos::bytes binaryCipher((cipher->len) / 2);
        OutputBuffer binaryBuffer{binaryCipher.data(), binaryCipher.size()};
        from_hex(&binaryBuffer, cipher);
        return g_ore_impl->decrypt4String(plain,
            bcos::bytesConstRef((bcos::byte*)key->data, key->len),
            bcos::bytesConstRef((bcos::byte*)binaryBuffer.data, binaryBuffer.len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief encrypt the plain data into cipher data using given key
 *
 * @param cipher the encrypted data
 * @param key the key used to encrypt
 * @param plain the plain-data
 */
void integer_fast_ore_encrypt(
    OutputBuffer* cipher, InputBuffer const* key, int64_t const& plain, bool hexEncode)
{
    clear_last_error();
    try
    {
        if (!hexEncode)
        {
            g_ore_impl->encrypt4Integer(
                cipher, bcos::bytesConstRef((bcos::byte*)key->data, key->len), plain);
            return;
        }

        auto hexedBufferSize = g_ore_impl->estimatedCipherSize(sizeof(plain), true);
        if (cipher->len < hexedBufferSize)
        {
            auto errorMsg =
                "integer_fast_ore_encrypt error for unenough cipher buffer, at least: " +
                std::to_string(hexedBufferSize);
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }

        bcos::bytes bytesResult(g_ore_impl->estimatedCipherSize(sizeof(plain), false));
        OutputBuffer bytesResultBuffer{bytesResult.data(), bytesResult.size()};
        g_ore_impl->encrypt4Integer(
            &bytesResultBuffer, bcos::bytesConstRef((bcos::byte*)key->data, key->len), plain);
        InputBuffer buffer{bytesResultBuffer.data, bytesResultBuffer.len};
        to_hex(cipher, &buffer);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief decrypt the cipher data into plain data using given key
 *
 * @param plain the decrypted plain data
 * @param key the key used to decrypt
 * @param cipher the cipher
 */
void integer_fast_ore_decrypt(
    int64_t* plain, InputBuffer const* key, InputBuffer const* cipher, bool hexEncode)
{
    clear_last_error();
    try
    {
        if (!hexEncode)
        {
            return g_ore_impl->decrypt4Integer(plain,
                bcos::bytesConstRef((bcos::byte*)key->data, key->len),
                bcos::bytesConstRef((bcos::byte*)cipher->data, cipher->len));
        }

        bcos::bytes binaryCipher((cipher->len) / 2);
        OutputBuffer binaryBuffer{binaryCipher.data(), binaryCipher.size()};
        from_hex(&binaryBuffer, cipher);
        return g_ore_impl->decrypt4Integer(plain,
            bcos::bytesConstRef((bcos::byte*)key->data, key->len),
            bcos::bytesConstRef((bcos::byte*)binaryBuffer.data, binaryBuffer.len));
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief encrypt the plain data into cipher data using given key
 *
 * @param cipher the encrypted data
 * @param key the key used to encrypt
 * @param plain the plain-data
 */
void float_fast_ore_encrypt(
    OutputBuffer* cipher, InputBuffer const* key, InputBuffer const* plain, bool hexEncode)
{
    clear_last_error();
    try
    {
        if (!hexEncode)
        {
            g_ore_impl->encrypt4Float(cipher, bcos::bytesConstRef((bcos::byte*)key->data, key->len),
                float50{std::string(plain->data, plain->data + plain->len)});
            return;
        }

        auto hexedBufferSize = g_ore_impl->estimatedFloatCipherSize(plain->len, true);
        if (cipher->len < hexedBufferSize)
        {
            auto errorMsg =
                "integer_fast_ore_encrypt error for unenough cipher buffer, at least: " +
                std::to_string(hexedBufferSize);
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }

        bcos::bytes bytesResult(g_ore_impl->estimatedFloatCipherSize(plain->len, false));
        OutputBuffer bytesResultBuffer{bytesResult.data(), bytesResult.size()};
        g_ore_impl->encrypt4Float(&bytesResultBuffer,
            bcos::bytesConstRef((bcos::byte*)key->data, key->len),
            float50{std::string(plain->data, plain->data + plain->len)});
        InputBuffer buffer{bytesResultBuffer.data, bytesResultBuffer.len};
        to_hex(cipher, &buffer);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief decrypt the cipher data into plain data using given key
 *
 * @param plain the decrypted plain data
 * @param key the key used to decrypt
 * @param cipher the cipher
 */
void float_fast_ore_decrypt(
    OutputBuffer* plain, InputBuffer const* key, InputBuffer const* cipher, bool hexEncode)
{
    clear_last_error();
    try
    {
        float50 res;
        if (!hexEncode)
        {
            res = g_ore_impl->decrypt4Float(bcos::bytesConstRef((bcos::byte*)key->data, key->len),
                bcos::bytesConstRef((bcos::byte*)cipher->data, cipher->len));
        }
        else
        {
            bcos::bytes binaryCipher((cipher->len) / 2);
            OutputBuffer binaryBuffer{binaryCipher.data(), binaryCipher.size()};
            from_hex(&binaryBuffer, cipher);
            res = g_ore_impl->decrypt4Float(bcos::bytesConstRef((bcos::byte*)key->data, key->len),
                bcos::bytesConstRef((bcos::byte*)binaryBuffer.data, binaryBuffer.len));
        }

        auto plainStr = res.str();
        if (plain->len < plainStr.size())
        {
            auto errorMsg = "float_fast_ore_decrypt error for unenough plain buffer, at least: " +
                            std::to_string(plainStr.size());
            set_last_error_msg(-1, errorMsg.c_str());
            return;
        }
        plain->len = plainStr.size();
        std::copy(plainStr.begin(), plainStr.end(), plain->data);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
}

/**
 * @brief compare between cipher1 and cipher2
 *
 * @param cipher1
 * @param cipher2
 * @return int
 */
int fast_ore_compare(InputBuffer const* cipher1, InputBuffer const* cipher2)
{
    clear_last_error();
    try
    {
        return g_ore_impl->compare(cipher1, cipher2);
    }
    catch (std::exception const& e)
    {
        auto errorMsg = boost::diagnostic_information(e);
        set_last_error_msg(-1, errorMsg.c_str());
    }
    return -1;
}