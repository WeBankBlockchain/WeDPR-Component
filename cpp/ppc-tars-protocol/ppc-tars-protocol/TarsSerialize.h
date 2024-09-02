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
 * @file TarsSerializable.h
 * @author: shawnhe
 * @date 2022-11-4
 */

#pragma once

#include "Common.h"
#include "TarsStruct.h"

namespace ppctars::serialize
{
void encode(TarsStruct auto const& object, bcos::bytes& out)
{
    tars::TarsOutputStream<BufferWriterByteVector> output;
    object.writeTo(output);
    output.getByteBuffer().swap(out);
}

void decode(const bcos::bytes& in, TarsStruct auto& out)
{
    tars::TarsInputStream<tars::BufferReader> input;
    input.setBuffer((const char*)in.data(), in.size());
    out.readFrom(input);
}

}  // namespace ppctars::serialize
