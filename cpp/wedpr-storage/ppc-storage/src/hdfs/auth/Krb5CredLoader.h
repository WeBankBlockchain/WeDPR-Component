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
 * @file Krb5CredLoader.h
 * @author: yujiechen
 * @date 2024-12-1
 */
#pragma once
#include "ppc-framework/protocol/Krb5AuthConfig.h"
#include <krb5.h>
#include <profile.h>
#include <memory>

namespace ppc::storage
{
class Krb5Context
{
public:
    using Ptr = std::shared_ptr<Krb5Context>;
    Krb5Context(ppc::protocol::Krb5AuthConfig::Ptr const& config) : m_config(config) {}

    virtual ~Krb5Context()
    {
        if (m_principal)
        {
            krb5_free_principal(m_ctx, m_principal);
        }
        if (m_creds)
        {
            krb5_free_cred_contents(m_ctx, m_creds);
        }
        if (m_ctx)
        {
            krb5_free_context(m_ctx);
        }
        if (m_profilePtr)
        {
            profile_release(m_profile);
        }
    }

    virtual void init();

private:
    void checkResult(krb5_error_code const& error, std::string const& method);

protected:
    ppc::protocol::Krb5AuthConfig::Ptr m_config;
    krb5_context m_ctx = NULL;
    profile_t m_profile;
    profile_t* m_profilePtr = NULL;
    krb5_principal m_principal = NULL;
    krb5_creds m_credsObj;
    krb5_creds* m_creds = NULL;
    krb5_ccache m_ccache = NULL;
};

class Krb5CredLoader
{
public:
    using Ptr = std::shared_ptr<Krb5CredLoader>;
    Krb5CredLoader() = default;
    virtual ~Krb5CredLoader() = default;

    virtual Krb5Context::Ptr load(ppc::protocol::Krb5AuthConfig::Ptr const& config) const
    {
        return std::make_shared<Krb5Context>(config);
    }
};
}  // namespace ppc::storage