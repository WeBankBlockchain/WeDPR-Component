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
 * @file HttpInterface.h
 * @author: zachma
 * @date 2023-8-34
 */

#pragma once
#include "../protocol/Protocol.h"
#include "../protocol/Task.h"
#include <bcos-boostssl/httpserver/Common.h>
#include <bcos-utilities/Error.h>
#include <bcos-utilities/Log.h>
#include <memory>

#define HTTP_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("HTTP")

namespace bcos
{
namespace boostssl
{
class MessageFace;
class WsSession;
}  // namespace boostssl
}  // namespace bcos
namespace ppc::http
{
using RespUrlFunc = std::function<void(bcos::Error::Ptr, bcos::bytes&&)>;
class HttpInterface
{
public:
    using Ptr = std::shared_ptr<HttpInterface>;
    HttpInterface() = default;
    virtual ~HttpInterface() = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void registerUrlHandler(std::string const& _urlName,
        std::function<void(bcos::boostssl::http::HttpRequest const&, RespUrlFunc)> _handler) = 0;
};
}  // namespace ppc::http