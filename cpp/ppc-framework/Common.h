/*
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
 * @file Common.h
 * @author: yujiechen
 * @date 2022-10-20
 */
#pragma once
#include <bcos-utilities/Exceptions.h>
#if ENABLE_CPU_FEATURES
#if X86
#include <cpu_features/cpuinfo_x86.h>
#endif
#if ARCH
#include <cpu_features/cpuinfo_aarch64.h>
#endif
#endif

#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN32_)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace ppc
{
/// construct a new exception class overriding Exception
#define DERIVE_PPC_EXCEPTION(X)        \
    struct X : virtual bcos::Exception \
    {                                  \
    }

#define CHECK_OFFSET_WITH_THROW_EXCEPTION(offset, length)                                    \
    do                                                                                       \
    {                                                                                        \
        if (offset > length)                                                                 \
        {                                                                                    \
            throw std::out_of_range("Out of range error, offset:" + std::to_string(offset) + \
                                    " ,length: " + std::to_string(length) +                  \
                                    " ,file: " + __FILE__ + " ,func: " + __func__ +          \
                                    " ,line: " + std::to_string(__LINE__));                  \
        }                                                                                    \
    } while (0);

DERIVE_PPC_EXCEPTION(OpenFileFailed);
DERIVE_PPC_EXCEPTION(DataSchemaNotSetted);
DERIVE_PPC_EXCEPTION(UnsupportedDataSchema);
DERIVE_PPC_EXCEPTION(WeDPRException);

#if ENABLE_CPU_FEATURES
#if X86
static const cpu_features::X86Features CPU_FEATURES = cpu_features::GetX86Info().features;
#endif
#if ARCH
static const cpu_features::Aarch64Features CPU_FEATURES = cpu_features::GetAarch64Info().features;
#endif
#endif

using u128 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<128, 128,
    boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

// for double support
using u1024 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<1024, 1024,
    boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

using s1024 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<1024, 1024,
    boost::multiprecision::signed_magnitude, boost::multiprecision::unchecked, void>>;

using float50 = boost::multiprecision::cpp_dec_float_50;
}  // namespace ppc