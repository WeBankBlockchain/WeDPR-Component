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
 * @file FrontServiceServer.h
 * @author: shawnhe
 * @date 2022-10-20
 */

#pragma once

#include "FrontService.h"
#include "libinitializer/FrontInitializer.h"
#include "ppc-protocol/src/PPCMessage.h"
#include "ppc-tars-protocol/ppc-tars-protocol/Common.h"

namespace ppctars
{
struct FrontServiceParam
{
    ppc::initializer::FrontInitializer::Ptr frontInitializer;
};

class FrontServiceServer : public FrontService
{
public:
    FrontServiceServer(FrontServiceParam const& _param)
      : m_frontInitializer(_param.frontInitializer)
    {}

    ~FrontServiceServer() override {}
    void initialize() override {}
    void destroy() override {}

public:
    ppctars::Error onReceiveMessage(
        const vector<tars::Char>& _message, tars::TarsCurrentPtr _current) override;

private:
    ppc::initializer::FrontInitializer::Ptr m_frontInitializer;
};
}  // namespace ppctars
