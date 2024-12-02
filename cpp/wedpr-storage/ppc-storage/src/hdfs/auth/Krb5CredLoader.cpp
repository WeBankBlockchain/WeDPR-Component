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
 * @file Krb5CredLoader.cpp
 * @author: yujiechen
 * @date 2024-12-1
 */
#include "Krb5CredLoader.h"
#include "../Common.h"

using namespace ppc::storage;
using namespace ppc;
using namespace ppc::protocol;
using namespace bcos;

void Krb5Context::init()
{
    HDFS_AUTH_LOG(INFO) << LOG_DESC("init Krb5Context") << m_config->desc();

    // init the profile
    auto ret = profile_init_path(m_config->authConfigFilePath.c_str(), &m_profile);
    if (ret)
    {
        BOOST_THROW_EXCEPTION(WeDPRException() << errinfo_comment(
                                  "load Krb5Context failed for profile_init_path failed!"));
    }
    m_profilePtr = &m_profile;
    // load krb5 ctx
    auto error = krb5_init_context_profile(m_profile, 1, &m_ctx);
    checkResult(error, "krb5_init_context_profile");

    // init the principal
    error = krb5_parse_name(m_ctx, m_config->principal.c_str(), &m_principal);
    checkResult(error, "krb5_parse_name");
    // init credential
    error = krb5_get_init_creds_password(
        m_ctx, &m_credsObj, m_principal, m_config->password.c_str(), NULL, NULL, 0, NULL, NULL);
    checkResult(error, "krb5_get_init_creds_password");
    m_creds = &m_credsObj;
    // init the ccache
    error = krb5_cc_resolve(m_ctx, m_config->ccachePath.c_str(), &m_ccache);
    checkResult(error, "krb5_cc_resolve");

    error = krb5_cc_initialize(m_ctx, m_ccache, m_principal);
    checkResult(error, "krb5_cc_initialize");
    // store the credential
    error = krb5_cc_store_cred(m_ctx, m_ccache, m_creds);
    HDFS_AUTH_LOG(INFO) << LOG_DESC("init Krb5Context success") << m_config->desc();
}

void Krb5Context::checkResult(krb5_error_code const& error, std::string const& method)
{
    if (!error)
    {
        HDFS_AUTH_LOG(INFO) << LOG_DESC("init Krb5Context: ") << method << " success";
        return;
    }
    auto msg = krb5_get_error_message(m_ctx, error);
    HDFS_AUTH_LOG(ERROR) << LOG_DESC("init Krb5Context failed") << LOG_KV("method", method)
                         << LOG_KV("reason", msg);
    BOOST_THROW_EXCEPTION(
        WeDPRException() << errinfo_comment(
            "load Krb5Context failed, method: " + method + ", reason: " + std::string(msg)));
}