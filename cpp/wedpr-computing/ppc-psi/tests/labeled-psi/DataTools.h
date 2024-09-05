/*
 *  Copyright (C) 2022 WeDPR.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicabl law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @file DataTools.cpp
 * @author: shawnhe
 * @date 2022-11-25
 */

#pragma
#include "ppc-framework/io/DataBatch.h"
#include <memory>
#include <string>

namespace ppc::psi
{
inline void genItemsLabels(
    ppc::io::DataBatch::Ptr _items, ppc::io::DataBatch::Ptr _labels, uint32_t _size)
{
    std::vector<bcos::bytes> items(_size);
    std::vector<bcos::bytes> labels(_size);
    for (uint32_t i = 0; i < _size; ++i)
    {
        std::string item = std::to_string(i);
        items[i] = bcos::bytes(item.begin(), item.end());
        labels[i] = bcos::bytes(item.begin(), item.end());
    }
    _items->setData(std::move(items));
    _labels->setData(std::move(labels));
}

}  // namespace ppc::psi
