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
 * @file Krb5AuthConfig.h
 * @author: yujiechen
 * @date 2024-12-2
 */
#pragma once
#include "ppc-framework/Common.h"
#include <memory>
#include <sstream>
#include <string>

namespace ppc::protocol
{
struct Krb5AuthConfig
{
    using Ptr = std::shared_ptr<Krb5AuthConfig>;
    std::string principal;
    std::string password;
    std::string ccachePath;
    std::string authConfigFilePath = "./conf/krb5.conf";
    void check() const
    {
        if (principal.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid krb5 auth config: Must set the principal!"));
        }
        if (password.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid krb5 auth config: Must set the password!"));
        }
        if (ccachePath.size() == 0)
        {
            BOOST_THROW_EXCEPTION(WeDPRException() << bcos::errinfo_comment(
                                      "Invalid krb5 auth config: Must set the ccachePath!"));
        }
        if (authConfigFilePath.size() == 0)
        {
            BOOST_THROW_EXCEPTION(
                WeDPRException() << bcos::errinfo_comment(
                    "Invalid krb5 auth config: Must set the authConfigFilePath!"));
        }
    }

    inline std::string desc() const
    {
        std::stringstream oss;
        oss << LOG_KV("principal", principal) << LOG_KV("ccachePath", ccachePath)
            << LOG_KV("authConfigFilePath", authConfigFilePath);
        return oss.str();
    }
};
}  // namespace ppc::protocol