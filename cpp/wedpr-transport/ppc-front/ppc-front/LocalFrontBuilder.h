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
 * @file LocalFrontBuilder.h
 * @author: yujiechen
 * @date 2024-09-4
 */
#pragma once
#include "ppc-framework/front/IFront.h"

namespace ppc::front
{
class LocalFrontBuilder : public IFrontBuilder
{
public:
    using Ptr = std::shared_ptr<LocalFrontBuilder>;
    LocalFrontBuilder(IFront::Ptr front) { m_front = front; }
    LocalFrontBuilder() = default;
    ~LocalFrontBuilder() override = default;

    IFrontClient::Ptr buildClient(std::string, std::function<void()> onUnHealthHandler,
        bool removeHandlerOnUnhealth) const override
    {
        return m_front;
    }

    void setFront(IFront::Ptr front) { m_front = std::move(front); }

private:
    IFront::Ptr m_front;
};
}  // namespace ppc::front