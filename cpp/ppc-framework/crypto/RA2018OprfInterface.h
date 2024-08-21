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
 * @file RA2018OprfInterface.h
 * @author: yujiechen
 * @date 2022-12-6
 */
#pragma once
#include "ppc-framework/io/DataBatch.h"
#include <bcos-utilities/Common.h>
namespace ppc::crypto
{
class RA2018OprfInterface
{
public:
    using Ptr = std::shared_ptr<RA2018OprfInterface>;
    RA2018OprfInterface() = default;
    virtual ~RA2018OprfInterface() = default;

    virtual bcos::bytes generatePrivateKey() = 0;
    virtual bcos::bytes inv(bcos::bytes const& _data) = 0;

    virtual void blind(ppc::io::DataBatch::Ptr const& _plainData, bcos::bytes const& _privateKey,
        std::vector<bcos::bytes>& _blindResult) = 0;

    virtual void finalize(std::vector<bcos::bytes> const& _evaluatedData,
        bcos::bytes const& _invPrivateKey, std::vector<bcos::bytes>& _finalizedResult) = 0;

    virtual void fullEvaluate(
        ppc::io::DataBatch::Ptr const& _input, std::vector<bcos::bytes>& _result) = 0;
    virtual void evaluate(
        std::vector<bcos::bytes> const& _blindData, std::vector<bcos::bytes>& _result) = 0;
};
}  // namespace ppc::crypto