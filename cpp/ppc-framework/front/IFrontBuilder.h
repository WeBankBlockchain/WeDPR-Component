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
 * @file IFront.h
 * @author: yujiechen
 * @date 2024-08-22
 */
#pragma once
#include "IFront.h"

namespace ppc::front
{
class IFrontBuilder
{
public:
    using Ptr = std::shared_ptr<IFrontBuilder>;
    IFrontBuilder() = default;
    virtual ~IFrontBuilder() = default;

    /**
     * @brief create the Front using specified config
     *
     * @param config the config used to build the Front
     * @return IFront::Ptr he created Front
     */
    virtual IFront::Ptr build(ppc::front::FrontConfig::Ptr config) const = 0;
    virtual IFrontClient::Ptr buildClient(std::string endPoint) const = 0;
};
}  // namespace ppc::front