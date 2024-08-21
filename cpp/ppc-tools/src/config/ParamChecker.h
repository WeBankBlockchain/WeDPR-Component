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
 * @file UrlChecker.h
 * @author: shawnhe
 * @date 2023-04-21
 */
#pragma once
#include "ppc-framework/Common.h"
#include <bcos-utilities/Log.h>

namespace ppc::tools
{
// ip:port or domain:port
inline bool checkEndpoint(const std::string& _endpoint)
{
    try
    {
        if (_endpoint.empty())
        {
            return false;
        }
        std::vector<std::string> url;
        boost::split(url, _endpoint, boost::is_any_of(":"), boost::token_compress_on);
        if (url.size() != 2)
        {
            return false;
        }

        int port = std::stoi(url[1]);
        if (0 > port || 65535 < port)
        {
            return false;
        }
    }
    catch (std::exception& e)
    {
        return false;
    }
    return true;
}
}  // namespace ppc::tools